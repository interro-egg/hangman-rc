#ifndef PLAYER_H
#define PLAYER_H

#include "../common/common.h"
#include <stdbool.h>

#define GS_DEFAULT_HOST "localhost"

#define INPUT_PROMPT "> "
#define MAX_COMMAND_NAME_SIZE 10 + 1
#define MAX_COMMAND_NAME_SIZE_FMT "%11s"

#define MSG_PARSE_ERROR "An error has occurred while parsing your command.\n"
#define MSG_EXEC_ERROR "An error has occurred while executing your command.\n"
#define MSG_UNKNOWN_COMMAND "Unknown command.\n"

typedef bool (*CommandHandler)(char *args);

// TODO: check if order here matches order in .c
void read_opts(int argc, char *argv[], char **host, char **port);
CommandHandler get_handler(char *cmd);
void dispatch(char *cmd, char *line);
char *findArgs(char *line, char *cmd);
bool mock_handler(char *args); // FIXME: delete this

#endif // PLAYER_H