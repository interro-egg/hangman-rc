#include "GS.h"
#include "../common/common.h"
#include "commands.h"
#include "network.h"
#include "persistence.h"
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

ServerState serverState = {NULL,  GS_DEFAULT_PORT,
                           false, NULL,
                           NULL,  NULL,
                           NULL,  -1,
                           NULL,  0,
                           NULL,  true,
                           0,     false};

int main(int argc, char *argv[]) {
    signal(SIGINT, handleGracefulShutdownSignal);
    signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE to avoid crashing on write
    signal(SIGCHLD, SIG_IGN); // explicitly ignore SIGCHLD to avoid zombies

    readOpts(argc, argv, &(serverState.word_file), &(serverState.port),
             &(serverState.verbose), &(serverState.sequential_word_selection));

    if (serverState.word_file == NULL) {
        fprintf(stderr, USAGE_FMT, argv[0]);
        exit(EXIT_FAILURE);
    }

    if (initPersistence() != 0) {
        fprintf(stderr, "Failed to access/create storage directories\n");
        destroyStateComponents(&serverState);
        exit(EXIT_FAILURE);
    }

    char inBuf[IN_BUFFER_SIZE] = {0};
    char outBuf[OUT_BUFFER_SIZE] = {0};
    serverState.in_buffer = inBuf;
    serverState.out_buffer = outBuf;

    int result = initNetwork(&serverState);
    if (result != NINIT_SUCCESS) {
        fprintf(stderr, "%s", translateNetworkInitError(result));
        destroyStateComponents(&serverState);
        exit(EXIT_FAILURE);
    }

    srand(RAND_SEED);
    serverState.word_list = parseWordListFile(serverState.word_file);
    if (serverState.word_list == NULL) {
        fprintf(stderr, "Failed to parse word list from file %s\n",
                serverState.word_file);
        destroyStateComponents(&serverState);
        exit(EXIT_FAILURE);
    }

    if (serverState.verbose) {
        printf("Word selection mode: %s\n",
               serverState.sequential_word_selection ? "SEQUENTIAL" : "RANDOM");
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("Failed to fork into UDP and TCP listeners");
        destroyStateComponents(&serverState);
        exit(EXIT_FAILURE);
    } else if (pid != 0) {
        // Parent process (UDP listener)

        result = initNetworkUDP(&serverState);
        if (result != NINIT_SUCCESS) {
            fprintf(stderr, "%s", translateNetworkInitError(result));
            destroyStateComponents(&serverState);
            exit(EXIT_FAILURE);
        }

        if (serverState.verbose) {
            printf("[UDP] Listening on port %s\n", serverState.port);
        }

        while (!serverState.shutdown) {
            ssize_t n = recvfrom(serverState.socket, serverState.in_buffer,
                                 IN_BUFFER_SIZE - 1, 0,
                                 (struct sockaddr *)serverState.player_addr,
                                 &serverState.player_addr_len);
            if (n <= 0) {
                if (errno != EINTR) {
                    perror(MSG_UDP_ERCV);
                }
                continue;
            }
            serverState.in_buffer[n] = '\0';

            logTraffic(T_REQUEST, "UDP", NULL, &serverState);

            if (serverState.in_buffer[n - 1] != '\n') {
                if (sprintf(serverState.out_buffer, "ERR\n") > 0) {
                    replyUDP(&serverState);
                }
                continue;
            }

            const UDPCommandDescriptor *descr =
                getUDPCommandDescriptor(serverState.in_buffer);
            if (descr == NULL) {
                if (sprintf(serverState.out_buffer, "ERR\n") > 0) {
                    replyUDP(&serverState);
                }
                continue;
            }

            char *EOL = strchr(serverState.in_buffer, '\n');
            if (EOL == NULL || strlen(EOL) != 1 ||
                (result = handleUDPCommand(descr, &serverState)) !=
                    HANDLER_SUCCESS) {
                if (sprintf(serverState.out_buffer, "%s ERR\n",
                            descr->response) > 0) {
                    replyUDP(&serverState);
                }

                if (result == HANDLER_ENOMEM) {
                    fprintf(stderr, MSG_NO_MEMORY);
                    destroyStateComponents(&serverState);
                    exit(EXIT_FAILURE);
                }
            }
        }
    } else {
        // Child process (TCP listener)

        result = initNetworkTCP(&serverState);
        if (result != NINIT_SUCCESS) {
            fprintf(stderr, "%s", translateNetworkInitError(result));
            destroyStateComponents(&serverState);
            exit(EXIT_FAILURE);
        }

        if (serverState.verbose) {
            printf("[TCP] Listening on port %s\n", serverState.port);
        }

        while (!serverState.shutdown) {
            int sessionFd = accept(serverState.socket,
                                   (struct sockaddr *)serverState.player_addr,
                                   &serverState.player_addr_len);

            if (sessionFd == -1) {
                if (errno != EINTR) {
                    perror(MSG_TCP_EACPT);
                }
                continue;
            }

            pid_t tcpPid = fork();

            if (tcpPid == -1) {
                perror(MSG_TCP_EFORK);
            } else if (tcpPid != 0) {
                // Parent process
                close(sessionFd);
                continue;
            } else {
                // Child process

                serverState.socket = sessionFd;

                if (initTCPSessionSocket(&serverState) != NINIT_SUCCESS ||
                    readTCPMessage(serverState.socket, serverState.in_buffer,
                                   IN_BUFFER_SIZE - 1) <= 0) {
                    if (sprintf(serverState.out_buffer, "ERR") > 0) {
                        replyTCP(NULL, &serverState);
                    }
                    destroyStateComponents(&serverState);
                    exit(EXIT_FAILURE);
                }

                logTraffic(T_REQUEST, "TCP", NULL, &serverState);

                const TCPCommandDescriptor *descr =
                    getTCPCommandDescriptor(serverState.in_buffer);
                if (descr == NULL) {
                    if (sprintf(serverState.out_buffer, "ERR") > 0) {
                        replyTCP(NULL, &serverState);
                    }

                    destroyStateComponents(&serverState);
                    exit(EXIT_FAILURE);
                }

                result = handleTCPCommand(descr, &serverState);
                if (result != HANDLER_SUCCESS) {
                    if (sprintf(serverState.out_buffer, "%s NOK",
                                descr->response) > 0) {
                        replyTCP(NULL, &serverState);
                    }

                    if (result == HANDLER_ENOMEM) {
                        fprintf(stderr, MSG_NO_MEMORY);
                    }

                    destroyStateComponents(&serverState);
                    exit(EXIT_FAILURE);
                }

                break;
            }
        }
    }

    destroyStateComponents(&serverState);
    return EXIT_SUCCESS;
}

void readOpts(int argc, char *argv[], char **word_file, char **port,
              bool *verbose, bool *sequential) {
    int opt;

    while ((opt = getopt(argc, argv, "-p:vr")) != -1) {
        switch (opt) {
        case 1:
            if (*word_file != NULL) {
                fprintf(stderr, USAGE_FMT, argv[0]);
                exit(EXIT_FAILURE);
            }
            if (access(optarg, R_OK) == -1) {
                fprintf(stderr, "%s: Cannot read from file.\n", optarg);
                exit(EXIT_FAILURE);
            }
            *word_file = optarg;
            break;
        case 'p':
            *port = optarg;
            break;
        case 'v':
            *verbose = true;
            break;
        case 'r':
            *sequential = false;
            break;
        default:
            fprintf(stderr, USAGE_FMT, argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

const UDPCommandDescriptor *getUDPCommandDescriptor(char *inBuf) {
    for (size_t i = 0; i < UDP_COMMANDS_COUNT; i++) {
        if (strncmp(inBuf, UDP_COMMANDS[i].name, COMMAND_NAME_SIZE) == 0) {
            // assumes all commands are the same length, COMMAND_NAME_SIZE
            if (inBuf[COMMAND_NAME_SIZE] != ' ' &&
                inBuf[COMMAND_NAME_SIZE] != '\n') {
                return NULL;
            }
            return &(UDP_COMMANDS[i]);
        }
    }
    return NULL;
}

const TCPCommandDescriptor *getTCPCommandDescriptor(char *inBuf) {
    for (size_t i = 0; i < TCP_COMMANDS_COUNT; i++) {
        if (strncmp(inBuf, TCP_COMMANDS[i].name, COMMAND_NAME_SIZE) == 0) {
            return &(TCP_COMMANDS[i]);
        }
    }
    return NULL;
}

char *translateNetworkInitError(int result) {
    switch (result) {
    case NINIT_ENOMEM:
        return MSG_NINIT_ENOMEM;
    case NINIT_UDP_EADDRINFO:
        return MSG_NINIT_UDP_EADDRINFO;
    case NINIT_UDP_ESOCKET:
        return MSG_NINIT_UDP_ESOCKET;
    case NINIT_UDP_ESNDTIMEO:
        return MSG_NINIT_UDP_ESNDTIMEO;
    case NINIT_UDP_EREUSEADDR:
        return MSG_NINIT_UDP_EREUSEADDR;
    case NINIT_UDP_EBIND:
        return MSG_NINIT_UDP_EBIND;
    case NINIT_TCP_EADDRINFO:
        return MSG_NINIT_TCP_EADDRINFO;
    case NINIT_TCP_ESOCKET:
        return MSG_NINIT_TCP_ESOCKET;
    case NINIT_TCP_ESNDTIMEO:
        return MSG_NINIT_TCP_ESNDTIMEO;
    case NINIT_TCP_EREUSEADDR:
        return MSG_NINIT_TCP_EREUSEADDR;
    case NINIT_TCP_EBIND:
        return MSG_NINIT_TCP_EBIND;
    case NINIT_TCP_ELISTEN:
        return MSG_NINIT_TCP_ELISTEN;
    default:
        return MSG_NINIT_EUNKNOWN;
    }
}

void handleGracefulShutdownSignal(int sig) {
    signal(sig, handleGracefulShutdownSignal);

    if (serverState.shutdown) {
        // second time
        destroyStateComponents(&serverState);
        exit(EXIT_FAILURE);
    }

    serverState.shutdown = true;

    while (1) {
        errno = 0;
        if (wait(NULL) == -1 && errno == ECHILD) {
            break;
        }
    }
}