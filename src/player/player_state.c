#include "player_state.h"
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>

// Only destroys State components; does not free(state).
// This must not be called before successful network init
void destroyStateComponents(PlayerState *state) {
    freeaddrinfo(state->udp_addr);
    freeaddrinfo(state->tcp_addr);
    free(state->timeout);
    close(state->udp_socket);
    free(state->PLID);
    free(state->word);
}