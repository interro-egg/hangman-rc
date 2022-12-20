#include "server_state.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Only destroys State components; does not free(state)
void destroyStateComponents(ServerState *state) {
    if (state->addr != NULL) {
        freeaddrinfo(state->addr);
    }
    free(state->timeout);
    if (state->socket >= 0) {
        close(state->socket);
    }
}

unsigned int calculateMaxErrors(char *word) {
    if (word == NULL) {
        return 0;
    }
    size_t len = strlen(word);
    if (len < 7) {
        return 7;
    } else if (len < 11) {
        return 8;
    } else {
        return 9;
    }
}