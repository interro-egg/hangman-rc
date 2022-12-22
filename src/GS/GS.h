#ifndef GS_H
#define GS_H

#include "commands.h"

#define RAND_SEED ((unsigned int)time(NULL))

#define USAGE_FMT "Usage: %s word_file [-p GSport] [-v]\n"

#define MSG_NO_MEMORY "No memory available.\n"
#define MSG_UDP_ERCV "Error: Could not receive UDP message.\n"
#define MSG_TCP_EACPT "Error: Could not accept TCP connection.\n"
#define MSG_TCP_EFORK "Error: Could not fork into TCP handler.\n"

// Network init error messages
#define MSG_NINIT_EUNKNOWN                                                     \
    "An error has occurred while initializing the game server's network "      \
    "features.\n"
#define MSG_NINIT_ENOMEM MSG_NO_MEMORY
#define MSG_NINIT_UDP_EADDRINFO "Error: Could not resolve UDP address.\n"
#define MSG_NINIT_TCP_EADDRINFO "Error: Could not resolve TCP address.\n"
#define MSG_NINIT_UDP_ESOCKET "Error: Could not create UDP socket.\n"
#define MSG_NINIT_TCP_ESOCKET "Error: Could not create TCP socket.\n"
#define MSG_NINIT_UDP_ESNDTIMEO                                                \
    "An error occurred while setting UDP send timeout value.\n"
#define MSG_NINIT_TCP_ESNDTIMEO                                                \
    "An error occurred while setting TCP send timeout value.\n"
#define MSG_NINIT_UDP_EREUSEADDR                                               \
    "An error occurred while setting UDP socket reuse address value.\n"
#define MSG_NINIT_TCP_EREUSEADDR                                               \
    "An error occurred while setting TCP socket reuse address value.\n"
#define MSG_NINIT_UDP_EBIND "An error occurred while binding UDP socket.\n"
#define MSG_NINIT_TCP_EBIND "An error occurred while binding TCP socket.\n"
#define MSG_NINIT_TCP_ELISTEN                                                  \
    "An error occurred while marking TCP socket as accepting connections.\n"

void readOpts(int argc, char *argv[], char **word_file, char **port,
              bool *verbose, bool *sequential);

const UDPCommandDescriptor *getUDPCommandDescriptor(char *inBuf);
const TCPCommandDescriptor *getTCPCommandDescriptor(char *inBuf);

char *translateNetworkInitError(int result);

void handleGracefulShutdownSignal(int sig);

#endif // GS_H