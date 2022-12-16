#define _GNU_SOURCE // needed for splice

#include "network.h"
#include "../common/common.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int initNetwork(PlayerState *state) {
    if (getAddrInfoSockType(state->host, state->port, &state->udp_addr,
                            SOCK_DGRAM, false) != 0) {
        return NINIT_EADDRINFO_UDP;
    }

    if (getAddrInfoSockType(state->host, state->port, &state->tcp_addr,
                            SOCK_STREAM, false) != 0) {
        return NINIT_EADDRINFO_TCP;
    }

    state->timeout = malloc(sizeof(struct timeval));
    if (state->timeout == NULL) {
        return NINIT_ENOMEM;
    }
    state->timeout->tv_sec = TIMEOUT_SECS;
    state->timeout->tv_usec = TIMEOUT_MICROSECS;

    state->udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (state->udp_socket == -1) {
        return NINIT_ESOCKET_UDP;
    }
    if (setsockopt(state->udp_socket, SOL_SOCKET, SO_RCVTIMEO, state->timeout,
                   sizeof(state->timeout)) != 0) {
        return NINIT_ERCVTIMEO_UDP;
    }
    if (setsockopt(state->udp_socket, SOL_SOCKET, SO_SNDTIMEO, state->timeout,
                   sizeof(state->timeout)) != 0) {
        return NINIT_ESNDTIMEO_UDP;
    }

    return NINIT_SUCCESS;
}

// Sends a message and waits for the response
int sendUDPMessage(PlayerState *state) {
    if (sendto(state->udp_socket, state->out_buffer, strlen(state->out_buffer),
               0, state->udp_addr->ai_addr,
               state->udp_addr->ai_addrlen) != -1 &&
        recvfrom(state->udp_socket, state->in_buffer, IN_BUFFER_SIZE, 0, NULL,
                 NULL) > 0) {
        return 0;
    }

    return -1;
}

// Sends a message and returns the socket for reading response.
// Must be closed by caller
int sendTCPMessage(PlayerState *state) {
    int fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    if (fd == -1) {
        return TCP_SND_ESOCKET;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, state->timeout,
                   sizeof(state->timeout)) != 0) {
        return TCP_SND_ERCVTIMEO;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, state->timeout,
                   sizeof(state->timeout)) != 0) {
        return TCP_SND_ESNDTIMEO;
    }

    if (connect(fd, state->tcp_addr->ai_addr, state->tcp_addr->ai_addrlen) ==
        -1) {
        close(fd);
        return TCP_SND_ECONNECT;
    }

    size_t toWrite = strlen(state->out_buffer);
    size_t written = 0;
    while (written < toWrite) {
        ssize_t n = write(fd, state->out_buffer + written, toWrite - written);
        if (n <= 0) {
            close(fd);
            return TCP_SND_EWRITE;
        }
        written += (size_t)n;
    }

    return fd;
}

// Fname Fsize Fdata, returns Fname
char *readFileTCP(int fd) {
    char fname[MAX_FNAME_LEN + 1];
    char fsize[MAX_FSIZE_LEN + 1];

    size_t alreadyReadName = 0;
    while (1) {
        ssize_t n = read(fd, fname + alreadyReadName, 1 * sizeof(char));
        if (n <= 0) {
            errno = TCP_RCV_EREAD;
            return NULL;
        }
        alreadyReadName += (size_t)n;

        if (fname[alreadyReadName - 1] == ' ') {
            break;
        } else if (alreadyReadName >= MAX_FNAME_LEN + 1) {
            errno = TCP_RCV_EINV;
            return NULL;
        }
    }
    fname[alreadyReadName - 1] = '\0';

    size_t alreadyReadSize = 0;
    while (1) {
        ssize_t n = read(fd, fsize + alreadyReadSize, 1 * sizeof(char));
        if (n <= 0) {
            errno = TCP_RCV_EREAD;
            return NULL;
        }
        alreadyReadSize += (size_t)n;

        if (fsize[alreadyReadSize - 1] == ' ') {
            break;
        } else if (!isdigit(fsize[alreadyReadSize - 1]) ||
                   alreadyReadSize >= MAX_FSIZE_LEN + 1) {
            errno = TCP_RCV_EINV;
            return NULL;
        }
    }
    fsize[alreadyReadSize - 1] = '\0';

    char *end;
    errno = 0;
    size_t fsizeNum = (size_t)strtoul(fsize, &end, 10);
    if (errno != 0 || *end != '\0' || fsizeNum > MAX_FSIZE_NUM) {
        errno = TCP_RCV_EINV;
        return NULL;
    }

    char *fnameAlloc = malloc(alreadyReadName * sizeof(char));
    if (fnameAlloc == NULL) {
        errno = TCP_RCV_ENOMEM;
        return NULL;
    }

    strncpy(fnameAlloc, fname, alreadyReadName - 1);
    fnameAlloc[alreadyReadName] = '\0';

    int fileFd = open(fnameAlloc, O_WRONLY | O_CREAT | O_TRUNC);
    if (fileFd == -1) {
        errno = TCP_RCV_EFOPEN;
        return NULL;
    }

    size_t remaining = fsizeNum;
    while (remaining > 0) {
        size_t n = MIN(FILE_TRANSFER_BLOCK_SIZE, remaining);
        ssize_t result = splice(fd, NULL, fileFd, NULL, n,
                                remaining > n ? SPLICE_F_MORE : 0);

        if (result <= 0) {
            errno = TCP_RCV_EFTRANSF;
            return NULL;
        }

        remaining -= (size_t)result;
    }

    char last;
    ssize_t n = read(fd, &last, 1 * sizeof(char));
    if (n <= 0 || last != '\n') {
        unlink(fnameAlloc);
        // no need to check for error as we're already returning error;
        // this is a best-effort attempt
        free(fnameAlloc);
        errno = TCP_RCV_EINV;
        return NULL;
    }

    return fnameAlloc;
}