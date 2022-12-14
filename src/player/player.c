#include "player.h"
#include "utils.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    char *host = GS_DEFAULT_HOST;
    char *port = GS_DEFAULT_PORT;

    readOpts(argc, argv, &host, &port);

    char *line = NULL;
    size_t bufSize;
    char *cmd = malloc(sizeof(char) * MAX_COMMAND_NAME_SIZE);
    char *outBuf = malloc(sizeof(char) * OUT_BUFFER_SIZE);
    if (cmd == NULL || outBuf == NULL) {
        free(cmd);
        free(outBuf);
        fprintf(stderr, "%s", MSG_NO_MEMORY);
        exit(EXIT_FAILURE);
    }

    PlayerState state = {host, port, false, NULL, NULL, outBuf, NULL};
    if (initUDPInfo(&state) == -1) {
        fprintf(stderr, "%s", MSG_UDP_CONNECTION_ERR);
        exit(EXIT_FAILURE);
    }
    while (printf(INPUT_PROMPT) >= 0 &&
           (getline(&line, &bufSize, stdin) != -1)) {
        size_t len = strlen(line);
        if (len <= 1) {
            continue;
        }
        line[len - 1] = '\0'; // remove trailing newline
        lowercase(line);      // playing is case insensitive

        dispatch(line, cmd, &state);

        putchar('\n');
    }
    free(line);
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
    const CommandDescriptor *desc;

    if (sscanf(line, MAX_COMMAND_NAME_SIZE_FMT, cmd) != 1) {
        printf(MSG_PARSE_ERROR);
    } else if ((desc = getCommandDescriptor(cmd)) == NULL) {
        printf(MSG_UNKNOWN_COMMAND);
    } else {
        int result;
        if ((result = handleCommand(desc, findArgs(line, cmd), state)) !=
            HANDLER_SUCCESS) {
            fprintf(stderr, "%s", translateHandlerError(result));

            if (result == HANDLER_ENOMEM) {
                exit(EXIT_FAILURE);
            }
        }
    }
}

const CommandDescriptor *getCommandDescriptor(char *cmd) {
    for (size_t i = 0; i < COMMANDS_COUNT; i++) {
        for (size_t j = 0; j < COMMANDS[i].aliasesCount; j++) {
            if (strcmp(cmd, COMMANDS[i].aliases[j]) == 0) {
                return &COMMANDS[i];
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

char *translateHandlerError(int result) {
    switch (result) {
    case HANDLER_EPARSE:
        return MSG_HANDLER_EPARSE;
    case HANDLER_ENOMEM:
        return MSG_HANDLER_ENOMEM;
    default:
        return MSG_HANDLER_EUNKNOWN;
    }
}