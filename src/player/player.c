#include "player.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    char *host = GS_DEFAULT_HOST;
    char *port = GS_DEFAULT_PORT;

    read_opts(argc, argv, &host, &port);

    char *line = NULL;
    size_t bufSize;
    char *cmd = malloc(sizeof(char) * MAX_COMMAND_NAME_SIZE);
    if (cmd == NULL) {
        exit_no_mem();
    }

    while (printf(INPUT_PROMPT) >= 0 &&
           (getline(&line, &bufSize, stdin) != -1)) {
        size_t len = strlen(line);
        if (len <= 1) {
            continue;
        }
        line[len - 1] = '\0'; // remove trailing newline
        lowercase(line);      // playing is case insensitive

        dispatch(line, cmd);

        putchar('\n');
    }
    free(line);
}

void read_opts(int argc, char *argv[], char **host, char **port) {
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

void dispatch(char *line, char *cmd) {
    CommandHandler handler;

    if (sscanf(line, MAX_COMMAND_NAME_SIZE_FMT, cmd) != 1) {
        printf(MSG_PARSE_ERROR);
    } else if ((handler = get_handler(cmd)) == NULL) {
        printf(MSG_UNKNOWN_COMMAND);
    } else if (!handler(findArgs(line, cmd))) {
        printf(MSG_EXEC_ERROR);
    }
}

CommandHandler get_handler(char *cmd) {
    if (strcmp(cmd, "start") == 0 || strcmp(cmd, "sg") == 0) {
        printf("<handle start command>\n");
        return mock_handler;
    } else if (strcmp(cmd, "play") == 0 || strcmp(cmd, "pl") == 0) {
        printf("<handle play command>\n");
        return mock_handler;
    } else if (strcmp(cmd, "guess") == 0 || strcmp(cmd, "gw") == 0) {
        printf("<handle guess command>\n");
        return mock_handler;
    } else if (strcmp(cmd, "scoreboard") == 0 || strcmp(cmd, "sb") == 0) {
        printf("<handle scoreboard command>\n");
        return mock_handler;
    } else if (strcmp(cmd, "hint") == 0 || strcmp(cmd, "h") == 0) {
        printf("<handle hint command>\n");
        return mock_handler;
    } else if (strcmp(cmd, "state") == 0 || strcmp(cmd, "st") == 0) {
        printf("<handle state command>\n");
        return mock_handler;
    } else if (strcmp(cmd, "quit") == 0) {
        printf("<handle quit command>\n");
        return mock_handler;
    } else if (strcmp(cmd, "exit") == 0) {
        printf("<handle exit command>\n");
        return mock_handler;
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

// FIXME: delete this
bool mock_handler(char *args) {
    printf("Mock: args = |%s|\n", args);
    return true;
}