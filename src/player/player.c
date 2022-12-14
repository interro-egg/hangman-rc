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
    CommandHandler handler;

    if (sscanf(line, MAX_COMMAND_NAME_SIZE_FMT, cmd) != 1) {
        printf(MSG_PARSE_ERROR);
    } else if ((handler = getHandler(cmd)) == NULL) {
        printf(MSG_UNKNOWN_COMMAND);
    } else {
        int result;
        if ((result = handler(findArgs(line, cmd), state)) != HANDLER_SUCCESS) {
            fprintf(stderr, translate_handler_error(result));

            if (result == HANDLER_ENOMEM) {
                exit(EXIT_FAILURE);
            }
        }
    }
}

CommandHandler getHandler(char *cmd) {
    if (strcmp(cmd, "start") == 0 || strcmp(cmd, "sg") == 0) {
        return startHandler;
    } else if (strcmp(cmd, "play") == 0 || strcmp(cmd, "pl") == 0) {
        return playHandler;
    } else if (strcmp(cmd, "guess") == 0 || strcmp(cmd, "gw") == 0) {
        return guessHandler;
    } else if (strcmp(cmd, "reveal") == 0 || strcmp(cmd, "rev") == 0) {
        return revealHandler;
    } else if (strcmp(cmd, "scoreboard") == 0 || strcmp(cmd, "sb") == 0) {
        return scoreboardHandler;
    } else if (strcmp(cmd, "hint") == 0 || strcmp(cmd, "h") == 0) {
        return hintHandler;
    } else if (strcmp(cmd, "state") == 0 || strcmp(cmd, "st") == 0) {
        return stateHandler;
    } else if (strcmp(cmd, "quit") == 0) {
        return quitHandler;
    } else if (strcmp(cmd, "exit") == 0) {
        return exitHandler;
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

char *translate_handler_error(int result) {
    switch (result) {
    case HANDLER_EPARSE:
        return MSG_HANDLER_EPARSE;
    case HANDLER_ENOMEM:
        return MSG_HANDLER_ENOMEM;
    default:
        return MSG_HANDLER_EUNKNOWN;
    }
}