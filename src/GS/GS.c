#include "GS.h"
#include "commands.h"
#include "network.h"
#include "persistence.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

ServerState serverState = {NULL,  GS_DEFAULT_PORT,
                           false, NULL,
                           NULL,  NULL,
                           NULL,  -1,
                           NULL,  0,
                           NULL,  true,
                           0};

int main(int argc, char *argv[]) {
    readOpts(argc, argv, &(serverState.word_file), &(serverState.port),
             &(serverState.verbose));

    if (serverState.word_file == NULL) {
        fprintf(stderr, USAGE_FMT, argv[0]);
        exit(EXIT_FAILURE);
    }

    if (ensureDirExists("GAMES") != 0 || ensureDirExists("SCORES") != 0) {
        fprintf(stderr, "Failed to access/create storage directories\n");
        exit(EXIT_FAILURE);
    }

    char inBuf[IN_BUFFER_SIZE] = {0};
    char outBuf[OUT_BUFFER_SIZE] = {0};
    serverState.in_buffer = inBuf;
    serverState.out_buffer = outBuf;

    int result = initNetwork(&serverState);
    if (result != NINIT_SUCCESS) {
        fprintf(stderr, "%s", translateNetworkInitError(result));
        exit(EXIT_FAILURE);
    }

    srand(RAND_SEED);
    umask(UMASK);
    serverState.word_list = parseWordListFile(serverState.word_file);
    if (serverState.word_list == NULL) {
        fprintf(stderr, "Failed to parse word list from file %s\n",
                serverState.word_file);
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("Failed to fork into UDP and TCP listeners");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child
        printf("This is the child!\n");
    } else {
        result = initNetworkUDP(&serverState);
        if (result != NINIT_SUCCESS) {
            fprintf(stderr, "%s", translateNetworkInitError(result));
            exit(EXIT_FAILURE);
        }

        while (1) {
            ssize_t n = recvfrom(serverState.socket, serverState.in_buffer,
                                 IN_BUFFER_SIZE, 0,
                                 (struct sockaddr *)serverState.player_addr,
                                 &serverState.player_addr_len);
            if (n <= 0) {
                perror(MSG_UDP_ERCV);
                continue;
            }
            serverState.in_buffer[n] = '\0';

            logRequest("UDP", &serverState);

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

            result = handleUDPCommand(descr, &serverState);
            if (result != HANDLER_SUCCESS) {
                if (sprintf(serverState.out_buffer, "%s ERR\n", descr->name) >
                    0) {
                    replyUDP(&serverState);
                }

                if (result == HANDLER_ENOMEM) {
                    fprintf(stderr, MSG_NO_MEMORY);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

    return EXIT_SUCCESS;
}

void readOpts(int argc, char *argv[], char **word_file, char **port,
              bool *verbose) {
    int opt;

    while ((opt = getopt(argc, argv, "-p:v")) != -1) {
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
        default:
            fprintf(stderr, USAGE_FMT, argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

int ensureDirExists(const char *path) {
    // check if directory exists
    if (access(path, R_OK || W_OK) == -1) {
        // if not, create it
        if (mkdir(path, 0755) == -1) {
            return -1;
        }
    }
    return 0;
}

const UDPCommandDescriptor *getUDPCommandDescriptor(char *inBuf) {
    for (size_t i = 0; i < UDP_COMMANDS_COUNT; i++) {
        if (strncmp(inBuf, UDP_COMMANDS[i].name, COMMAND_NAME_SIZE) == 0) {
            return &(UDP_COMMANDS[i]);
        }
    }
    printf("Unknown command: %s", inBuf);
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
    default:
        return MSG_NINIT_EUNKNOWN;
    }
}