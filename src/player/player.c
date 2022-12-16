#include "player.h"
#include "network.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    char *host = GS_DEFAULT_HOST;
    char *port = GS_DEFAULT_PORT;

    readOpts(argc, argv, &host, &port);

    size_t bufSize;
    char cmd[MAX_COMMAND_NAME_SIZE] = {0};
    char inBuf[IN_BUFFER_SIZE] = {0};
    char outBuf[OUT_BUFFER_SIZE] = {0};

    PlayerState state = {host,   port, NULL,  NULL, NULL, -1, -1, inBuf,
                         outBuf, NULL, false, NULL, NULL, 0,  0};

    int result;
    if ((result = initNetwork(&state)) == NINIT_SUCCESS) {
        fprintf(stderr, "%s", translateNetworkInitError(result));
        exit(EXIT_FAILURE);
    }

    while (printf(INPUT_PROMPT) >= 0 &&
           (getline(&state.line, &bufSize, stdin) != -1)) {
        size_t len = strlen(state.line);
        if (len <= 1) {
            continue;
        }
        state.line[len - 1] = '\0'; // remove trailing newline
        lowercase(state.line);      // playing is case insensitive

        dispatch(state.line, cmd, &state);

        putchar('\n');
    }

    destroyStateComponents(&state);
}

void readOpts(int argc, char *argv[], char **host, char **port) {
    int opt;

    while ((opt = getopt(argc, argv, "n:p:")) != -1) {
        switch (opt) {
        case 'n':
            *host = optarg;
            break;
        case 'p':
            *port = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s [-n GSIP] [-p GSport]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

void dispatch(char *line, char *cmd, PlayerState *state) {
    const UDPCommandDescriptor *desc;

    if (sscanf(line, MAX_COMMAND_NAME_SIZE_FMT, cmd) != 1) {
        printf(MSG_PARSE_ERROR);
    } else if ((desc = getUDPCommandDescriptor(cmd)) == NULL) {
        printf(MSG_UNKNOWN_COMMAND);
    } else {
        int result;
        if ((result = handleUDPCommand(desc, findArgs(line, cmd), state)) !=
            HANDLER_SUCCESS) {
            fprintf(stderr, "%s", translateHandlerError(result));

            if (result == HANDLER_ENOMEM) {
                exit(EXIT_FAILURE);
            }
        }
    }
}

const UDPCommandDescriptor *getUDPCommandDescriptor(char *cmd) {
    for (size_t i = 0; i < UDP_COMMANDS_COUNT; i++) {
        for (size_t j = 0; j < UDP_COMMANDS[i].aliasesCount; j++) {
            if (strcmp(cmd, UDP_COMMANDS[i].aliases[j]) == 0) {
                return &UDP_COMMANDS[i];
            }
        }
    }
    return NULL;
}

char *findArgs(char *line, char *cmd) {
    size_t len = strlen(cmd);
    if (line[len] == ' ' && line[len + 1] != '\0') {
        return line + len + 1;
    } else {
        return NULL;
    }
    // this intentionally counts "cmd " as having no args
    // FIXME: report no args if there are only spaces after cmd
}

char *translateNetworkInitError(int result) {
    switch (result) {
    case NINIT_ENOMEM:
        return MSG_NINIT_ENOMEM;
    case NINIT_EADDRINFO_UDP:
        return MSG_NINIT_EADDRINFO_UDP;
    case NINIT_EADDRINFO_TCP:
        return MSG_NINIT_EADDRINFO_TCP;
    case NINIT_ESOCKET_UDP:
        return MSG_NINIT_ESOCKET_UDP;
    case NINIT_ERCVTIMEO_UDP:
        return MSG_NINIT_ERCVTIMEO_UDP;
    case NINIT_ESNDTIMEO_UDP:
        return MSG_NINIT_ESNDTIMEO_UDP;
    default:
        return MSG_NINIT_EUNKNOWN;
    }
}

char *translateHandlerError(int result) {
    switch (result) {
    case HANDLER_ENOMEM:
        return MSG_HANDLER_ENOMEM;
    case HANDLER_EPARSE:
        return MSG_HANDLER_EPARSE;
    case HANDLER_ESERIALIZE:
        return MSG_HANDLER_ESERIALIZE;
    case HANDLER_ECOMMS:
        return MSG_HANDLER_ECOMMS;
    case HANDLER_ECOMMS_TIMEO:
        return MSG_HANDLER_ECOMMS_TIMEO;
    case HANDLER_EDESERIALIZE:
        return MSG_HANDLER_EDESERIALIZE;
    default:
        return MSG_HANDLER_EUNKNOWN;
    }
}