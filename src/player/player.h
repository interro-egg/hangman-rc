#ifndef PLAYER_H
#define PLAYER_H

#include "../common/common.h"
#include "commands.h"
#include "player_state.h"

#define GS_DEFAULT_HOST "localhost"

#define INPUT_PROMPT "> "
#define MAX_COMMAND_NAME_SIZE 10 + 1
#define MAX_COMMAND_NAME_SIZE_FMT "%11s"

#define MSG_PARSE_ERROR "An error has occurred while parsing your command.\n"
#define MSG_EXEC_ERROR "An error has occurred while executing your command.\n"
#define MSG_UNKNOWN_COMMAND "Unknown command.\n"

typedef bool (*CommandHandler)(char *args, PlayerState *state);

// TODO: check if order here matches order in .c
void readOpts(int argc, char *argv[], char **host, char **port);
CommandHandler getHandler(char *cmd);
void dispatch(char *cmd, char *line, PlayerState *state);
char *findArgs(char *line, char *cmd);

#endif // PLAYER_H