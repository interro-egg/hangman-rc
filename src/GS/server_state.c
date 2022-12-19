#include "server_state.h"
#include <stdlib.h>
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