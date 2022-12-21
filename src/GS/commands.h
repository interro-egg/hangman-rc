#ifndef COMMANDS_H
#define COMMANDS_H

#include "../common/messages.h"
#include "persistence.h"
#include "server_state.h"
#include <sys/types.h>

#define COMMAND_NAME_SIZE 3

#define HANDLER_SUCCESS 0
#define HANDLER_EUNKNOWN -1
#define HANDLER_ENOMEM -2
#define HANDLER_EDESERIALIZE -3
#define HANDLER_EFULFILL -4
#define HANDLER_ESERIALIZE -5
#define HANDLER_ECOMMS -6
#define HANDLER_EFILE_REQUIRED -7

typedef void *(*UDPRequestFulfiller)(void *req, ServerState *state);
typedef int (*TCPRequestFulfiller)(void *req, ServerState *state,
                                   ResponseFile **fptr);

typedef struct {
    char *name;
    MessageDeserializer requestDeserializer;
    MessageDestroyer requestDestroyer;
    UDPRequestFulfiller requestFulfiller;
    MessageSerializer responseSerializer;
    MessageDestroyer responseDestroyer;
    char *response;
} UDPCommandDescriptor;

typedef struct {
    char *name;
    MessageDeserializer requestDeserializer;
    MessageDestroyer requestDestroyer;
    TCPRequestFulfiller requestFulfiller;
    const char **statusEnumStrings;
    const bool *fileSendStatuses;
    char *response;
} TCPCommandDescriptor;

extern const UDPCommandDescriptor UDP_COMMANDS[];
extern const size_t UDP_COMMANDS_COUNT;

extern const TCPCommandDescriptor TCP_COMMANDS[];
extern const size_t TCP_COMMANDS_COUNT;

int handleUDPCommand(const UDPCommandDescriptor *cmd, ServerState *state);
int handleTCPCommand(const TCPCommandDescriptor *cmd, ServerState *state);

void *fulfillSNGRequest(void *req, ServerState *state);
void *fulfillPLGRequest(void *req, ServerState *state);
void *fulfillPWGRequest(void *req, ServerState *state);
void *fulfillQUTRequest(void *req, ServerState *state);
void *fulfillREVRequest(void *req, ServerState *state);

int fulfillGSBRequest(void *req, ServerState *state, ResponseFile **fptr);
int fulfillGHLRequest(void *req, ServerState *state, ResponseFile **fptr);

#endif // COMMANDS_H