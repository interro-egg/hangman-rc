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
                   sizeof(*(state->timeout))) != 0) {
        printf("Error: %d", errno);
        return NINIT_ERCVTIMEO_UDP;
    }
    if (setsockopt(state->udp_socket, SOL_SOCKET, SO_SNDTIMEO, state->timeout,
                   sizeof(*(state->timeout))) != 0) {
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
    state->tcp_socket = fd;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, state->timeout,
                   sizeof(*(state->timeout))) != 0) {
        return TCP_SND_ERCVTIMEO;
    }
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, state->timeout,
                   sizeof(*(state->timeout))) != 0) {
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

// maxLen does NOT include null terminator.
// returns number of bytes read, including space (converted to \0)
ssize_t readWordTCP(int fd, char *buf, size_t maxLen, bool checkDigits) {
    size_t alreadyRead = 0;
    while (1) {
        ssize_t n = read(fd, buf + alreadyRead, 1 * sizeof(char));
        if (n <= 0) {
            return (errno == EINPROGRESS) ? TCP_RCV_ETIMEO : TCP_RCV_EREAD;
        }
        alreadyRead += (size_t)n;

        if (buf[alreadyRead - 1] == ' ') {
            break;
        } else if (alreadyRead >= maxLen + 1 ||
                   (checkDigits && !isdigit(buf[alreadyRead - 1]))) {
            return TCP_RCV_EINV;
        }
    }
    buf[alreadyRead - 1] = '\0';

    return (ssize_t)alreadyRead;
}

// Fname Fsize Fdata, returns Fname
ReceivedFile *readFileTCP(int fd) {
    char fname[MAX_FNAME_LEN + 1];
    char fsize[MAX_FSIZE_LEN + 1];
    char transfBuf[FILE_TRANSFER_BLOCK_SIZE];

    ssize_t r = readWordTCP(fd, fname, MAX_FNAME_LEN, false);
    if (r <= 0) {
        errno = (int)r;
        return NULL;
    }
    size_t fnameLen = (size_t)r - 1;

    r = readWordTCP(fd, fsize, MAX_FSIZE_LEN, true);
    if (r <= 0) {
        errno = (int)r;
        return NULL;
    }

    ReceivedFile *file = malloc(sizeof(ReceivedFile));
    if (file == NULL) {
        errno = TCP_RCV_ENOMEM;
        return NULL;
    }

    char *end;
    errno = 0;
    file->fsize = (size_t)strtoul(fsize, &end, 10);
    if (errno != 0 || *end != '\0' || file->fsize > MAX_FSIZE_NUM) {
        errno = TCP_RCV_EINV;
        return NULL;
    }

    file->fname = malloc((fnameLen + 1) * sizeof(char));
    if (file->fname == NULL) {
        errno = TCP_RCV_ENOMEM;
        return NULL;
    }
    strncpy(file->fname, fname, fnameLen + 1);

    FILE *fileFd = fopen(file->fname, "w");
    if (fileFd == NULL) {
        errno = TCP_RCV_EFOPEN;
        return NULL;
    }

    size_t remaining = file->fsize;
    while (remaining > 0) {
        size_t amt = MIN(FILE_TRANSFER_BLOCK_SIZE, remaining);

        size_t already_read = 0;
        while (already_read < amt) {
            errno = 0;
            ssize_t result = read(fd, transfBuf + already_read, amt - already_read);
            if (result <= 0) {
                bool timedOut = errno == EINPROGRESS;
                destroyReceivedFile(file);
                fclose(fileFd);
                errno = timedOut ? TCP_RCV_ETIMEO : TCP_RCV_EFTRANSF;
                return NULL;
            }
            already_read += (size_t)result;
        }

        size_t already_written = 0;
        while (already_written < already_read) {
            size_t result = fwrite(transfBuf, sizeof(char), amt, fileFd);
            if (result == 0) {
                destroyReceivedFile(file);
                fclose(fileFd);
                errno = TCP_RCV_EFTRANSF;
                return NULL;
            }
            already_written += result;
        }

        remaining -= already_written;
    }

    fclose(fileFd);

    return file;
}

int checkNewline(int fd) {
    char c;
    ssize_t n = read(fd, &c, 1 * sizeof(char));
    if (n <= 0 || c != '\n') {
        return -1;
    }
    return 0;
}

void destroyReceivedFile(ReceivedFile *file) {
    if (file != NULL) {
        free(file->fname);
    }
    free(file);
}