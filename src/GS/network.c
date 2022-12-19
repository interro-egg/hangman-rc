#include "network.h"
#include "../common/common.h"
#include "errno.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

int initNetworkTimeout(struct timeval **timeout) {
    struct timeval *val = malloc(sizeof(struct timeval));
    if (timeout == NULL) {
        return NINIT_ENOMEM;
    }
    val->tv_sec = TIMEOUT_SECS;
    val->tv_usec = TIMEOUT_MICROSECS;
    *timeout = val;
    return NINIT_SUCCESS;
}

int initNetworkUDP(ServerState *state) {
    if (getAddrInfoSockType(NULL, state->port, &state->addr, SOCK_DGRAM,
                            true) != 0) {
        return NINIT_UDP_EADDRINFO;
    }

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        return NINIT_UDP_ESOCKET;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, state->timeout,
                   sizeof(*(state->timeout))) != 0) {
        return NINIT_UDP_ESNDTIMEO;
    }
    const bool reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) != 0) {
        printf("errno = |%d|\n", errno);
        return NINIT_UDP_EREUSEADDR;
    }

    if (bind(fd, state->addr->ai_addr, state->addr->ai_addrlen) != 0) {
        return NINIT_UDP_EBIND;
    }

    state->socket = fd;
    return NINIT_SUCCESS;
}