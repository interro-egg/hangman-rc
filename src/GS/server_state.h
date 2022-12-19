#ifndef SERVER_STATE_H
#define SERVER_STATE_H

#include <netdb.h>
#include <stdbool.h>
#include <sys/time.h>

typedef struct {
    char *word_file;
    char *port;
    bool verbose;

    char *in_buffer;
    char *out_buffer;

    // free with freeaddrinfo(addr);
    struct addrinfo *addr;
    struct timeval *timeout;
    int socket;
} ServerState;

void destroyStateComponents(ServerState *state);

#endif // SERVER_STATE_H