#include "common.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void lowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = (char)tolower(str[i]);
    }
}

int parseEnum(const char *strings[], char *toParse) {
    for (unsigned int i = 0; strings[i] != NULL; i++) {
        if (strcmp(strings[i], toParse) == 0) {
            return (int)i;
        }
    };
    return -1;
}

int getAddrInfoSockType(char *host, char *port, struct addrinfo **addr,
                        int sockType, bool passive) {
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = sockType;
    if (passive) {
        hints.ai_flags = AI_PASSIVE;
    }

    return getaddrinfo(host, port, &hints, addr);
}

int checkPLID(char *PLID) {
    if (strlen(PLID) != PLID_SIZE) {
        return -1;
    }
    for (unsigned int i = 0; i < PLID_SIZE; i++) {
        if (!isdigit(PLID[i])) {
            return -1;
        }
    }
    return 0;
}