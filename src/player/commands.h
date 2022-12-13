#ifndef COMMANDS_H
#define COMMANDS_H

#include "../common/common.h"
#include "player_state.h"
#include <stdbool.h>

// Command handlers return false if an unexpected error ocurred, otherwise
// (if the command was successful or if the error was expected and dealt
// with) they return true.

bool startHandler(char *args, PlayerState *state);
bool playHandler(char *args, PlayerState *state);
bool guessHandler(char *args, PlayerState *state);
bool revealHandler(char *args, PlayerState *state);
bool scoreboardHandler(char *args, PlayerState *state);
bool hintHandler(char *args, PlayerState *state);
bool stateHandler(char *args, PlayerState *state);
bool quitHandler(char *args, PlayerState *state);
bool exitHandler(char *args, PlayerState *state);

#endif // COMMANDS_H