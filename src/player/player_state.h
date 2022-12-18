#ifndef PLAYER_STATE_H
#define PLAYER_STATE_H

#include <netdb.h>
#include <stdbool.h>
#include <sys/time.h>

typedef struct {
    char *host;
    char *port;

    // free with freeaddrinfo(addr);
    struct addrinfo *udp_addr;
    struct addrinfo *tcp_addr;
    struct timeval *timeout;
    int udp_socket;
    int tcp_socket; // current, if any

    char *in_buffer;
    char *out_buffer;
    char *line;

    bool in_game;
    char *PLID;
    char *word;
    unsigned int remaining_errors;
    unsigned int trial;

    bool shutdown;
} PlayerState;

void destroyStateComponents(PlayerState *state);
int startGame(PlayerState *state, unsigned int n_letters,
              unsigned int max_errors, char *PLID);
void endGame(PlayerState *state);

#endif // PLAYER_STATE_H