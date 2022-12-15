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

    char *in_buffer;
    char *out_buffer;

    bool in_game;
    char *PLID;
    char *word;
    unsigned int max_errors;
    unsigned int trial;
} PlayerState;

void destroyStateComponents(PlayerState *state);

#endif // PLAYER_STATE_H