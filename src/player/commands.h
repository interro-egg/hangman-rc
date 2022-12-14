#ifndef COMMANDS_H
#define COMMANDS_H

#include "../common/common.h"
#include "player_state.h"
#include <stdbool.h>

#define HANDLER_SUCCESS 0
#define HANDLER_EUNKNOWN -1
#define HANDLER_EPARSE -2
#define HANDLER_ENOMEM -3

int startHandler(char *args, PlayerState *state);
int playHandler(char *args, PlayerState *state);
int guessHandler(char *args, PlayerState *state);
int revealHandler(char *args, PlayerState *state);
int scoreboardHandler(char *args, PlayerState *state);
int hintHandler(char *args, PlayerState *state);
int stateHandler(char *args, PlayerState *state);
int quitHandler(char *args, PlayerState *state);
int exitHandler(char *args, PlayerState *state);

#endif // COMMANDS_H