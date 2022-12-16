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
    strncpy(state->PLID, PLID, 6 + 1);
    state->in_game = true;
    return 0;
}

void endGame(PlayerState *state) {
    state->in_game = false;
    free(state->word);
    state->word = NULL;
}

void displayWord(char *word) {
    if (word == NULL) {
        printf("No word found.\n");
        return;
    }
    for (size_t i = 0; word[i] != '\0'; i++) {
        printf("%s%c", i != 0 ? " " : "", word[i]);
    }
}