#ifndef NETWORK_H
#define NETWORK_H

#include "player_state.h"

#define TIMEOUT_MS 3000
#define MAX_TRIES 5

char *sendUDPMessage(PlayerState *state, char *req);
int initUDPInfo(PlayerState *state);
int sendTCPMessage(PlayerState *state, char *req);

#endif // NETWORK_H