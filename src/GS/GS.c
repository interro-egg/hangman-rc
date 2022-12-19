#include "GS.h"
#include "network.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

ServerState serverState = {NULL, GS_DEFAULT_PORT, false, NULL, NULL, NULL, NULL,
                           -1};

int main(int argc, char *argv[]) {
    readOpts(argc, argv, &(serverState.word_file), &(serverState.port),
             &(serverState.verbose));

    if (serverState.word_file == NULL) {
        fprintf(stderr, USAGE_FMT, argv[0]);
        exit(EXIT_FAILURE);
    }

    char inBuf[IN_BUFFER_SIZE] = {0};
    char outBuf[OUT_BUFFER_SIZE] = {0};
    serverState.in_buffer = inBuf;
    serverState.out_buffer = outBuf;

    int result = initNetworkTimeout(&(serverState.timeout));
    if (result != NINIT_SUCCESS) {
        fprintf(stderr, "%s", translateNetworkInitError(result));
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
            struct sockaddr_in playerAddr;
            socklen_t playerAddrLen = sizeof(playerAddr);
            ssize_t n = recvfrom(
                serverState.socket, serverState.in_buffer, IN_BUFFER_SIZE, 0,
                (struct sockaddr *)&playerAddr, &playerAddrLen);
            if (n <= 0) {
                perror(MSG_UDP_ERCV);
                continue;
            }
            serverState.in_buffer[n] = '\0';

            logRequest("UDP", &playerAddr, playerAddrLen, &serverState);
            if (serverState.in_buffer[n - 1] != '\n') {
                // TODO: send "ERR"
                continue;
            }

            UDPCommandDescriptor *descr = getUDPCommandDescriptor(
                serverState.in_buffer, (size_t)n, &serverState);
            if (descr == NULL) {
                // TODO: send "ERR"
                continue;
            }

            result = handleUDPCommand(descr, &serverState);
            if (result != HANDLER_SUCCESS) {
                // TODO: send "[name] ERR"

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

UDPCommandDescriptor *getUDPCommandDescriptor(UNUSED char *inBuf,
                                              UNUSED size_t len,
                                              UNUSED ServerState *state) {
    // TODO: implement
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