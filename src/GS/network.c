#include "network.h"
#include "../common/common.h"
#include "errno.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int initNetwork(ServerState *state) {
    struct timeval *val = malloc(sizeof(struct timeval));
    if (val == NULL) {
        return NINIT_ENOMEM;
    }
    val->tv_sec = TIMEOUT_SECS;
    val->tv_usec = TIMEOUT_MICROSECS;
    state->timeout = val;

    state->player_addr = malloc(sizeof(struct sockaddr_in));
    if (state->player_addr == NULL) {
        return NINIT_ENOMEM;
    }
    state->player_addr_len = sizeof(struct sockaddr_in);

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
        return NINIT_UDP_EREUSEADDR;
    }

    if (bind(fd, state->addr->ai_addr, state->addr->ai_addrlen) != 0) {
        return NINIT_UDP_EBIND;
    }

    state->socket = fd;
    return NINIT_SUCCESS;
}

int replyUDP(ServerState *state) {
    return sendto(state->socket, state->out_buffer, strlen(state->out_buffer),
                  0, (struct sockaddr *)state->player_addr,
                  state->player_addr_len) > 0
               ? 0
               : -1;
}

void logRequest(char *proto, ServerState *state) {
    if (!state->verbose) {
        return;
    }

    char playerIpAddr[INET_ADDRSTRLEN] = "???.???.???.???";
    inet_ntop(state->player_addr->sin_family, &(state->player_addr->sin_addr),
              playerIpAddr,
              INET_ADDRSTRLEN); // no need to check error

    char host[MAX_HOSTNAME_SIZE];
    char playerHostname[MAX_HOSTNAME_SIZE + 3];
    if (getnameinfo((struct sockaddr *)state->player_addr,
                    state->player_addr_len, host, MAX_HOSTNAME_SIZE, NULL, 0,
                    NI_NAMEREQD) == 0) {
        snprintf(playerHostname, MAX_HOSTNAME_SIZE + 3, "(%s) ", host);
    }
    printf("[RCV] [%s] [%s%s:%d]: %s\n", proto, playerHostname, playerIpAddr,
           ntohs(state->player_addr->sin_port), state->in_buffer);
}