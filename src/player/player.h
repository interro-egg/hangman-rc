#ifndef PLAYER_H
#define PLAYER_H

#include "../common/common.h"
#include "commands.h"
#include "player_state.h"

#define GS_DEFAULT_HOST "localhost"

#define INPUT_PROMPT "> "
#define MAX_COMMAND_NAME_SIZE 10 + 1
#define MAX_COMMAND_NAME_SIZE_FMT "%11s"
#define OUT_BUFFER_SIZE 128 + 1

#define MSG_NO_MEMORY "No memory available.\n"
#define MSG_PARSE_ERROR                                                        \
    "An error has occurred while parsing your command. Please make sure it "   \
    "is correctly formatted.\n"
#define MSG_UNKNOWN_COMMAND "Unknown command.\n"
#define MSG_UDP_CONNECTION_ERR                                                 \
    "An error has occurred while connecting to the game server.\n"

// Handler error messages
#define MSG_HANDLER_EUNKNOWN                                                   \
    "An error has occurred while executing your command.\n"
#define MSG_HANDLER_EPARSE MSG_PARSE_ERROR
#define MSG_HANDLER_ENOMEM MSG_NO_MEMORY

// TODO: check if order here matches order in .c
void readOpts(int argc, char *argv[], char **host, char **port);
const CommandDescriptor *getCommandDescriptor(char *cmd);
void dispatch(char *cmd, char *line, PlayerState *state);
char *findArgs(char *line, char *cmd);
char *translateHandlerError(int result);

#endif // PLAYER_H