#ifndef NETWORK_H
#define NETWORK_H

#include "server_state.h"

#define IN_BUFFER_SIZE 128 + 1
#define OUT_BUFFER_SIZE 128 + 1

#define TIMEOUT_SECS 5
#define TIMEOUT_MICROSECS 0

#define MAX_HOSTNAME_SIZE 64 + 1

#define NINIT_SUCCESS 0
#define NINIT_ENOMEM -1
#define NINIT_UDP_EADDRINFO -2
#define NINIT_UDP_ESOCKET -3
#define NINIT_UDP_ESNDTIMEO -4
#define NINIT_UDP_EREUSEADDR -5
#define NINIT_UDP_EBIND -6

int initNetworkTimeout(struct timeval **timeout);
int initNetworkUDP(ServerState *state);

void logRequest(char *proto, struct sockaddr_in *playerAddr,
                socklen_t playerAddrLen, ServerState *state);

#endif // NETWORK_H