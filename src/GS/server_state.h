#ifndef SERVER_STATE_H
#define SERVER_STATE_H

#include <netdb.h>
#include <stdbool.h>
#include <sys/time.h>

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

    struct WordList *word_list;
    bool sequential_word_selection;
    size_t word_list_seq_ptr;
} ServerState;

void destroyStateComponents(ServerState *state);
unsigned int calculateMaxErrors(char *word);
unsigned int getLetterPositions(char letter, char* word, unsigned int **pos);

#endif // SERVER_STATE_H