#ifndef NETWORK_H
#define NETWORK_H

#include "player_state.h"
#include <sys/types.h>

#define FILE_TRANSFER_BLOCK_SIZE 512

#define IN_BUFFER_SIZE 128 + 1
#define OUT_BUFFER_SIZE 128 + 1

// The combination of TIMEOUT_SECS and TIMEOUT_MICROSECS forms the timeout value
// If the timeout value is zero, requests will never timeout
#define TIMEOUT_SECS 5
#define TIMEOUT_MICROSECS 0

#define NINIT_SUCCESS 0
#define NINIT_ENOMEM -1
#define NINIT_EADDRINFO_UDP -2
#define NINIT_EADDRINFO_TCP -3
#define NINIT_ESOCKET_UDP -4
#define NINIT_ERCVTIMEO_UDP -5
#define NINIT_ESNDTIMEO_UDP -6

#define TCP_SND_ESOCKET -1
#define TCP_SND_ERCVTIMEO -2
#define TCP_SND_ESNDTIMEO -3
#define TCP_SND_ECONNECT -4
#define TCP_SND_EWRITE -5

#define TCP_RCV_EINV -1
#define TCP_RCV_EREAD -2
#define TCP_RCV_ETIMEO -3
#define TCP_RCV_EFOPEN -4
#define TCP_RCV_EFTRANSF -5
#define TCP_RCV_ENOMEM -6

typedef struct {
    char *fname;
    size_t fsize;
} ReceivedFile;

int initNetwork(PlayerState *state);

int sendUDPMessage(PlayerState *state);
int sendTCPMessage(PlayerState *state);
ssize_t readWordTCP(int fd, char *buf, size_t maxLen, bool checkDigits,
                    bool *foundEnd);
ReceivedFile *readFileTCP(int fd);
int checkNewline(int fd);
void destroyReceivedFile(ReceivedFile *file);

#endif // NETWORK_H