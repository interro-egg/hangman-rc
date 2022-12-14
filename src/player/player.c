#include "player.h"
#include "network.h"
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PlayerState playerState = {NULL, NULL, NULL,  NULL, NULL, -1, -1, NULL,
                           NULL, NULL, false, NULL, NULL, 0,  0,  false};

int main(int argc, char *argv[]) {
    signal(SIGINT, handleGracefulShutdownSignal);
    signal(SIGPIPE, SIG_IGN);

    char *host = GS_DEFAULT_HOST;
    char *port = GS_DEFAULT_PORT;

    readOpts(argc, argv, &host, &port);

    size_t bufSize;
    char inBuf[IN_BUFFER_SIZE] = {0};
    char outBuf[OUT_BUFFER_SIZE] = {0};

    playerState.host = host;
    playerState.port = port;
    playerState.in_buffer = inBuf;
    playerState.out_buffer = outBuf;

    int result;
    if ((result = initNetwork(&playerState)) != NINIT_SUCCESS) {
        fprintf(stderr, "%s", translateNetworkInitError(result));
        destroyStateComponents(&playerState);
        exit(EXIT_FAILURE);
    }

    displayTitle();
    displayHelp();

    while (!(playerState.shutdown) && printf(INPUT_PROMPT) >= 0 &&
           (getline(&playerState.line, &bufSize, stdin) != -1)) {
        size_t len = strlen(playerState.line);
        if (len <= 1) {
            continue;
        }
        playerState.line[len - 1] = '\0'; // remove trailing newline
        lowercase(playerState.line);      // playing is case insensitive

        dispatch(playerState.line, &playerState);

        putchar('\n');
    }

    gracefulShutdown(EXIT_SUCCESS);
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

void dispatch(char *line, PlayerState *state) {
    char cmd[MAX_COMMAND_NAME_SIZE] = {0};
    const UDPCommandDescriptor *descUDP = NULL;
    const TCPCommandDescriptor *descTCP = NULL;
    int result = HANDLER_EUNKNOWN;

    if (sscanf(line, MAX_COMMAND_NAME_SIZE_FMT, cmd) != 1) {
        fprintf(stderr, MSG_PARSE_ERROR);
    } else if (strcmp(line, "help") == 0) {
        displayHelp();
        return;
    } else if ((descUDP = getCommandDescriptor(
                    cmd, (const void *)UDP_COMMANDS, UDP_COMMANDS_COUNT,
                    UDPCommandDescriptorsIndexer, getUDPCommandAliases,
                    getUDPCommandAliasesCount)) != NULL) {
        result = handleUDPCommand(descUDP, findArgs(line, cmd), state);
    } else if ((descTCP = getCommandDescriptor(
                    cmd, (const void *)TCP_COMMANDS, TCP_COMMANDS_COUNT,
                    TCPCommandDescriptorsIndexer, getTCPCommandAliases,
                    getTCPCommandAliasesCount)) != NULL) {
        result = handleTCPCommand(descTCP, findArgs(line, cmd), state);
    } else {
        fprintf(stderr, MSG_UNKNOWN_COMMAND);
        return;
    }

    if (result != HANDLER_SUCCESS) {
        fprintf(stderr, "%s", translateHandlerError(result));

        if (result == HANDLER_ENOMEM) {
            // no memory to dispatch an EXIT_COMMAND
            destroyStateComponents(state);
            exit(EXIT_FAILURE);
        }
    }
}

const void *getCommandDescriptor(char *cmd, const void *commandsArr,
                                 const size_t commandsCount,
                                 CommandDescriptorsIndexer indexer,
                                 CommandAliasesGetter aliasesGetter,
                                 CommandAliasesCountGetter aliasesCountGetter) {
    for (size_t i = 0; i < commandsCount; i++) {
        const void *desc = indexer(commandsArr, i);
        char **aliases = aliasesGetter(desc);
        size_t aliasesCount = aliasesCountGetter(desc);
        for (size_t j = 0; j < aliasesCount; j++) {
            if (strcmp(cmd, aliases[j]) == 0) {
                return desc;
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
}

void displayTitle() {
    printf(
        " _   _    _    _   _  ____ __  __    _    _   _           ____   "
        "____\n| | | |  / \\  | \\ | |/ ___|  \\/  |  / \\  | \\ | |         | "
        " "
        "_ \\ / ___|\n| |_| | / _ \\ |  \\| | |  _| |\\/| | / _ \\ |  \\| |  "
        "_____  | |_) | |    \n|  _  |/ ___ \\| |\\  | |_| | |  | |/ ___ \\| "
        "|\\  | |_____| |  _ <| |___ \n|_| |_/_/   \\_\\_| \\_|\\____|_|  "
        "|_/_/  "
        " \\_\\_| \\_|         |_| \\_\\____|\n");
}

void displayHelp() {
    printf("\nAvailable commands:\n");
    for (size_t i = 0; i < UDP_COMMANDS_COUNT; i++) {
        UDPCommandDescriptor desc = UDP_COMMANDS[i];
        printf("\t- %s: %s\n", desc.usage, desc.description);
    }
    printf("\t- help: Display this list of commands\n\n");
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
    case HANDLER_ECOMMS_UDP:
        return MSG_HANDLER_ECOMMS_UDP;
    case HANDLER_ECOMMS_TCP:
        return MSG_HANDLER_ECOMMS_TCP;
    case HANDLER_ECOMMS_TIMEO:
        return MSG_HANDLER_ECOMMS_TIMEO;
    case HANDLER_EDESERIALIZE:
        return MSG_HANDLER_EDESERIALIZE;
    default:
        return MSG_HANDLER_EUNKNOWN;
    }
}

void gracefulShutdown(int retcode) {
    if (playerState.in_game) {
        dispatch(EXIT_COMMAND, &playerState);
    }
    destroyStateComponents(&playerState);
    exit(retcode);
}

void handleGracefulShutdownSignal(int sig) {
    signal(sig, handleGracefulShutdownSignal);
    gracefulShutdown(EXIT_FAILURE);
}