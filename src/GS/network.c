#define _GNU_SOURCE // for NI_MAXHOST from netdb.h

#include "network.h"
#include "../common/common.h"
#include "errno.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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
        close(fd);
        return NINIT_UDP_ESNDTIMEO;
    }
    const bool reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) != 0) {
        close(fd);
        return NINIT_UDP_EREUSEADDR;
    }

    if (bind(fd, state->addr->ai_addr, state->addr->ai_addrlen) != 0) {
        close(fd);
        return NINIT_UDP_EBIND;
    }

    state->socket = fd;
    return NINIT_SUCCESS;
}

int initNetworkTCP(ServerState *state) {
    if (getAddrInfoSockType(NULL, state->port, &state->addr, SOCK_STREAM,
                            true) != 0) {
        return NINIT_TCP_EADDRINFO;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        return NINIT_TCP_ESOCKET;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, state->timeout,
                   sizeof(*(state->timeout))) != 0) {
        close(fd);
        return NINIT_TCP_ESNDTIMEO;
    }
    const bool reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) != 0) {
        close(fd);
        return NINIT_TCP_EREUSEADDR;
    }

    if (bind(fd, state->addr->ai_addr, state->addr->ai_addrlen) != 0) {
        close(fd);
        return NINIT_TCP_EBIND;
    }

    if (listen(fd, TCP_SOCKET_BACKLOG) != 0) {
        close(fd);
        return NINIT_TCP_ELISTEN;
    }

    state->socket = fd;
    return NINIT_SUCCESS;
}

int initTCPSessionSocket(ServerState *state) {
    if (setsockopt(state->socket, SOL_SOCKET, SO_SNDTIMEO, state->timeout,
                   sizeof(*(state->timeout))) != 0) {
        return NINIT_TCP_ESNDTIMEO;
    }
    if (setsockopt(state->socket, SOL_SOCKET, SO_RCVTIMEO, state->timeout,
                   sizeof(*(state->timeout))) != 0) {
        return NINIT_TCP_ERCVTIMEO;
    }

    return NINIT_SUCCESS;
}

int replyUDP(ServerState *state) {
    logTraffic(T_RESPONSE, "UDP", NULL, state);

    return sendto(state->socket, state->out_buffer, strlen(state->out_buffer),
                  0, (struct sockaddr *)state->player_addr,
                  state->player_addr_len) > 0
               ? 0
               : -1;
}

// Sends state->out_buffer, followed by the file (if exists), followed by \n
int replyTCP(ResponseFile *file, ServerState *state) {
    logTraffic(T_RESPONSE, "TCP", file, state);

    if (writeToTCPSocket(state->socket, state->out_buffer,
                         strlen(state->out_buffer)) == -1) {
        return -1;
    }

    if (file != NULL) {
        sprintf(state->out_buffer, " %s %lu ", file->name, file->size);
        if (writeToTCPSocket(state->socket, state->out_buffer,
                             strlen(state->out_buffer)) == -1 ||
            writeToTCPSocket(state->socket, file->data, file->size) == -1) {
            return -1;
        }
    }

    char end = '\n';
    if (writeToTCPSocket(state->socket, &end, sizeof(char)) == -1) {
        return -1;
    }

    return 0;
}

int writeToTCPSocket(int socket, char *buf, size_t toWrite) {
    size_t written = 0;

    while (written < toWrite) {
        ssize_t sent = write(socket, buf + written,
                             MIN(TCP_MAX_BLOCK_SIZE, toWrite - written));
        if (sent <= 0) {
            return -1;
        }
        written += (size_t)sent;
    }

    return 0;
}

ssize_t readTCPMessage(int socket, char *buf, size_t bufSize) {
    size_t alreadyRead = 0;
    ssize_t n;
    while (alreadyRead < bufSize &&
           (n = read(socket, buf + alreadyRead, sizeof(char))) > 0) {
        alreadyRead += (size_t)n;
        if (buf[alreadyRead - 1] == '\n') {
            return (ssize_t)alreadyRead;
        }
    }
    return -1;
}

void logTraffic(enum TrafficType type, char *proto, ResponseFile *file,
                ServerState *state) {
    if (!state->verbose) {
        return;
    }

    char playerIpAddr[INET_ADDRSTRLEN] = "???.???.???.???";
    inet_ntop(state->player_addr->sin_family, &(state->player_addr->sin_addr),
              playerIpAddr,
              INET_ADDRSTRLEN); // no need to check error

    char host[NI_MAXHOST];
    char playerHostname[NI_MAXHOST + 3] = {0};
    if (getnameinfo((struct sockaddr *)state->player_addr,
                    state->player_addr_len, host, NI_MAXHOST, NULL, 0,
                    NI_NAMEREQD) == 0) {
        snprintf(playerHostname, NI_MAXHOST + 3, "(%s) ", host);
    }

    char *buf = type == T_REQUEST ? state->in_buffer : state->out_buffer;
    printf("[%s] [%s] [%s%s:%d]: %s", type == T_REQUEST ? "RCV" : "SND", proto,
           playerHostname, playerIpAddr, ntohs(state->player_addr->sin_port),
           buf);

    if (type == T_RESPONSE && strcmp(proto, "TCP") == 0) {
        if (file != NULL) {
            printf(" %s %lu <data>\n", file->name, file->size);
        } else {
            putchar('\n');
        }
    } else if (buf[strlen(buf) - 1] != '\n') {
        printf("§\n"); // indicate message had no newline
    }
}