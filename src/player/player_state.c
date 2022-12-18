#include "player_state.h"
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Only destroys State components; does not free(state)
void destroyStateComponents(PlayerState *state) {
    if (state->udp_addr != NULL) {
        freeaddrinfo(state->udp_addr);
    }
    if (state->tcp_addr != NULL) {
        freeaddrinfo(state->tcp_addr);
    }
    free(state->timeout);
    if (state->udp_socket != -1) {
        close(state->udp_socket);
    }
    if (state->tcp_socket != -1) {
        close(state->tcp_socket);
    }
    free(state->line);
    free(state->PLID);
    free(state->word);
}

int startGame(PlayerState *state, unsigned int n_letters,
              unsigned int max_errors, char *PLID) {
    state->word = malloc((n_letters + 1) * sizeof(char));
    if (state->word == NULL) {
        return -1;
    }
    for (unsigned int i = 0; i < n_letters; i++) {
        state->word[i] = '_';
    }
    state->word[n_letters] = '\0';
    state->remaining_errors = max_errors;
    state->trial = 0;
    state->PLID = malloc((6 + 1) * sizeof(char));
    if (state->word == NULL) {
        return -1;
    }
    strncpy(state->PLID, PLID, 6);
    state->PLID[6] = '\0';
    state->in_game = true;
    return 0;
}

void endGame(PlayerState *state) {
    state->in_game = false;
    free(state->word);
    state->word = NULL;
}