#ifndef SERVER_STATE_H
#define SERVER_STATE_H

#include <netdb.h>
#include <stdbool.h>
#include <sys/time.h>


//FIXME: This is a circular dependency, it's been put here for now
typedef struct {
    char *word;
    char *hintFile;
} WordListEntry;

typedef struct {
    size_t numEntries;
    WordListEntry *entries;
} WordList;

typedef struct {
    char *word_file;
    char *port;
    bool verbose;

    char *in_buffer;
    char *out_buffer;

    // free with freeaddrinfo(addr);
    struct addrinfo *addr;
    struct timeval *timeout;
    int socket;

    struct sockaddr_in *player_addr;
    socklen_t player_addr_len;

    WordList *word_list; // Circular dependency
    bool randomize_word_list;
} ServerState;

void destroyStateComponents(ServerState *state);

#endif // SERVER_STATE_H