#ifndef NETWORK_H
#define NETWORK_H

#include "commands.h"
#include "server_state.h"

#define IN_BUFFER_SIZE 128 + 1
#define OUT_BUFFER_SIZE 128 + 1
#define TCP_MAX_BLOCK_SIZE 512

// The combination of TIMEOUT_SECS and TIMEOUT_MICROSECS forms the timeout value
// If the timeout value is zero, requests will never timeout
#define TIMEOUT_SECS 5
#define TIMEOUT_MICROSECS 0

#define TCP_SOCKET_BACKLOG 5

#define NINIT_SUCCESS 0
#define NINIT_ENOMEM -1
#define NINIT_UDP_EADDRINFO -2
#define NINIT_UDP_ESOCKET -3
#define NINIT_UDP_ESNDTIMEO -4
#define NINIT_UDP_EREUSEADDR -5
#define NINIT_UDP_EBIND -6
#define NINIT_TCP_EADDRINFO -7
#define NINIT_TCP_ESOCKET -8
#define NINIT_TCP_ESNDTIMEO -9
#define NINIT_TCP_ERCVTIMEO -10
#define NINIT_TCP_EREUSEADDR -11
#define NINIT_TCP_EBIND -12
#define NINIT_TCP_ELISTEN -13

int initNetwork(ServerState *state);
int initNetworkUDP(ServerState *state);
int initNetworkTCP(ServerState *state);
int initTCPSessionSocket(ServerState *state);

int replyUDP(ServerState *state);
int replyTCP(ResponseFile *file, ServerState *state);
int writeToTCPSocket(int socket, char *buf, size_t toWrite);
ssize_t readTCPMessage(int socket, char *buf, size_t bufSize);

enum TrafficType { T_REQUEST, T_RESPONSE };

void logTraffic(enum TrafficType type, char *proto, ResponseFile *file,
                ServerState *state);

#endif // NETWORK_H