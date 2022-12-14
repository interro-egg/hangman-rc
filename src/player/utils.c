#include "utils.h"

#include <netdb.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

char *sendUDPMessage(PlayerState *state, char *req) {
    char *resp = malloc(128 * sizeof(char));
    struct pollfd fd;
    fd.events = POLLIN;
    int ret, tries = 0;

    int sock = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
    if (sock == -1)                            /*error*/
        return NULL;
    fd.fd = sock;

    if (sendto(sock, req, strlen(req), 0, state->addr->ai_addr,
               state->addr->ai_addrlen) == -1) {
        close(sock);
        return NULL;
    }

    while (ret == 0) {
        ret = poll(&fd, 1, TIMEOUT_MS);
        if (tries >= MAX_TRIES) {
            close(sock);
            return NULL;
        }
        tries++;
    }
    if (ret > 0) {
        if (recvfrom(sock, resp, 128, 0, NULL, NULL) == -1) {
            close(sock);
            return NULL;
        }
        close(sock);
        return resp;
    }
    close(sock);
    return NULL;
}

int initUDPInfo(PlayerState *state) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    if (getaddrinfo(state->host, state->port, &hints, &state->addr) != 0) {
        return -1;
    }
    return 1;
}