#include "server_state.h"
#include <malloc.h>
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

unsigned int getLetterPositions(char letter, char *word, unsigned int **pos) {
    if (word == NULL) {
        return 0;
    }
    size_t len = strlen(word);
    *pos = malloc(sizeof(unsigned int) * (len + 1));
    if (*pos == NULL) {
        return 0;
    }
    unsigned int n = 0;
    for (unsigned int i = 0; i < len; i++) {
        if (word[i] == letter) {
            (*pos)[n] = i + 1;
            n++;
        }
    }
    if (n == 0) {
        free(*pos);
        return 0;
    }

    *pos = realloc(*pos, sizeof(unsigned int) * n);
    if (*pos == NULL) {
        return 0;
    }
    return n;
}