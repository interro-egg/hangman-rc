#ifndef COMMANDS_H
#define COMMANDS_H

#include "../common/common.h"
#include "player_state.h"
#include <stdbool.h>

// Command handlers return false if an unexpected error ocurred, otherwise
// (if the command was successful or if the error was expected and dealt
// with) they return true.

bool start_handler(char *args, PlayerState *state);
bool play_handler(char *args, PlayerState *state);
bool guess_handler(char *args, PlayerState *state);
bool reveal_handler(char *args, PlayerState *state);
bool scoreboard_handler(char *args, PlayerState *state);
bool hint_handler(char *args, PlayerState *state);
bool state_handler(char *args, PlayerState *state);
bool quit_handler(char *args, PlayerState *state);
bool exit_handler(char *args, PlayerState *state);

#endif // COMMANDS_H