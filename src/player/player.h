#ifndef PLAYER_H
#define PLAYER_H

#include "../common/common.h"
#include "commands.h"
#include "player_state.h"

#define GS_DEFAULT_HOST "localhost"

#define INPUT_PROMPT "> "
#define MAX_COMMAND_NAME_SIZE 10 + 1
#define MAX_COMMAND_NAME_SIZE_FMT "%11s"

#define MSG_NO_MEMORY "No memory available.\n"
#define MSG_UNKNOWN_COMMAND "Unknown command.\n"
#define MSG_PARSE_ERROR                                                        \
    "Could not parse your command: please make sure your input is valid and "  \
    "correctly formatted.\n"
#define MSG_COMMS_ERROR                                                        \
    "An error has occurred while communicating with the game server. Please "  \
    "try again.\n"

// Network init error messages
#define MSG_NINIT_EUNKNOWN                                                     \
    "An error has occurred while connecting to the game server.\n"
#define MSG_NINIT_ENOMEM MSG_NO_MEMORY
#define MSG_NINIT_EADDRINFO_UDP "Error: Could not resolve UDP address.\n"
#define MSG_NINIT_EADDRINFO_TCP "Error: Could not resolve TCP address.\n"
#define MSG_NINIT_ESOCKET_UDP "Error: Could not create UDP socket.\n"
#define MSG_NINIT_ERCVTIMEO_UDP                                                \
    "An error occurred while setting UDP receive timeout value.\n"
#define MSG_NINIT_ESNDTIMEO_UDP                                                \
    "An error occurred while setting UDP send timeout value.\n"

// Handler error messages
#define MSG_HANDLER_EUNKNOWN                                                   \
    "An error has occurred while executing your command.\n"
#define MSG_HANDLER_ENOMEM MSG_NO_MEMORY
#define MSG_HANDLER_EPARSE MSG_PARSE_ERROR
#define MSG_HANDLER_ESERIALIZE                                                 \
    "An error has occurred while serializing your command.\n"
#define MSG_HANDLER_ECOMMS_UDP MSG_COMMS_ERROR
#define MSG_HANDLER_ECOMMS_TCP MSG_COMMS_ERROR
#define MSG_HANDLER_ECOMMS_TIMEO                                               \
    "Connection timed out while communicating with the game server. Please "   \
    "try again.\n"
#define MSG_HANDLER_EDESERIALIZE                                               \
    "An invalid response was received from the game server. Please try "       \
    "again.\n"

// TODO: check if order here matches order in .c
void readOpts(int argc, char *argv[], char **host, char **port);
const void *getCommandDescriptor(char *cmd, const void *commandsArr,
                                 const size_t commandsCount,
                                 CommandDescriptorsIndexer indexer,
                                 CommandAliasesGetter aliasesGetter,
                                 CommandAliasesCountGetter aliasesCountGetter);
void dispatch(char *cmd, char *line, PlayerState *state);
char *findArgs(char *line, char *cmd);
char *translateNetworkInitError(int result);
char *translateHandlerError(int result);

#endif // PLAYER_H