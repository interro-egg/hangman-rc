#include "commands.h"
#include <stdio.h>

int handleUDPCommand(UDPCommandDescriptor *cmd, ServerState *state) {
    void *req = cmd->requestDeserializer(state->in_buffer);
    if (req == NULL) {
        return HANDLER_EDESERIALIZE;
    }

    void *resp = cmd->requestFulfiller(req, state);
    if (resp == NULL) {
        cmd->requestDestroyer(req);
        return HANDLER_EFULFILL;
    }

    if (cmd->responseSerializer(resp, state->out_buffer) <= 0) {
        cmd->requestDestroyer(req);
        cmd->responseDestroyer(resp);
        return HANDLER_ESERIALIZE;
    }

    // TODO: Send response
    printf("Response: %s", state->out_buffer);

    return HANDLER_SUCCESS;
}