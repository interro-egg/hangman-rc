#ifndef GS_H
#define GS_H

#include "../common/common.h"
#include "server_state.h"

#define USAGE_FMT "Usage: %s word_file [-p GSport] [-v]\n"

#define MSG_NO_MEMORY "No memory available.\n"

// Network init error messages
#define MSG_NINIT_EUNKNOWN                                                     \
    "An error has occurred while initializing the game server's network "      \
    "features.\n"
#define MSG_NINIT_ENOMEM MSG_NO_MEMORY
#define MSG_NINIT_UDP_EADDRINFO "Error: Could not resolve UDP address.\n"
#define MSG_NINIT_TCP_EADDRINFO "Error: Could not resolve TCP address.\n"
#define MSG_NINIT_UDP_ESOCKET "Error: Could not create UDP socket.\n"
#define MSG_NINIT_UDP_ESNDTIMEO                                                \
    "An error occurred while setting UDP send timeout value.\n"
#define MSG_NINIT_UDP_EREUSEADDR                                               \
    "An error occurred while setting UDP socket reuse address value.\n"
#define MSG_NINIT_UDP_EBIND "An error occurred while binding UDP socket.\n"

void readOpts(int argc, char *argv[], char **word_file, char **port,
              bool *verbose);
char *translateNetworkInitError(int result);

#endif // GS_H