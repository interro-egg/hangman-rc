#ifndef PLAYER_STATE_H
#define PLAYER_STATE_H

#include <stdbool.h>

typedef struct {
    char *host;
    char *port;
    bool in_game;
    char *PLID;
    char *word;
    char *out_buffer;
    struct addrinfo *addr; // free with freeaddrinfo(addr);
} PlayerState;

#endif // PLAYER_STATE_H