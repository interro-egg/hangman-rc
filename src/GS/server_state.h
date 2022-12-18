#ifndef SERVER_STATE_H
#define SERVER_STATE_H

#include <stdbool.h>

typedef struct {
    char *word_file;
    char *port;
    bool verbose;
} ServerState;

// void destroyStateComponents(ServerState *state);

#endif // SERVER_STATE_H