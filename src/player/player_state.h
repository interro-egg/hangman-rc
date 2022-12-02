#ifndef PLAYER_STATE_H
#define PLAYER_STATE_H

#include <stdbool.h>

typedef struct {
    bool in_game;
    char *PLID;
    char *word;
} PlayerState;

#endif // PLAYER_STATE_H