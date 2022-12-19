#include "commands.h"
#include "../common/common.h"
#include <stdio.h>
#include <stdlib.h>

const UDPCommandDescriptor SNGCmd = {"SNG",
                                     deserializeSNGMessage,
                                     destroySNGMessage,
                                     fulfillSNGRequest,
                                     serializeRSGMessage,
                                     destroyRSGMessage};

/*
const UDPCommandDescriptor PLGCmd = {"PLG",
                                     deserializePLGMessage,
                                     destroyRLGMessage,
                                     fulfillPLGRequest,
                                     serializeRSGMessage,
                                     destroyRSGMessage};

const UDPCommandDescriptor PWGCmd = {"PWG",
                                     deserializePWGMessage,
                                     destroyPWGMessage,
                                     fulfillPWGRequest,
                                     serializeRWGMessage,
                                     destroyRWGMessage};

const UDPCommandDescriptor QUTCmd = {"QUT",
                                     deserializeQUTMessage,
                                     destroyQUTMessage,
                                     fulfillQUTRequest,
                                     serializeRQTMessage,
                                     destroyRQTMessage};

const UDPCommandDescriptor REVCmd = {"REV",
                                     deserializeREVMessage,
                                     destroyREVMessage,
                                     fulfillREVRequest,
                                     serializeRRVMessage,
                                     destroyRRVMessage};
*/
const UDPCommandDescriptor UDP_COMMANDS[] = {SNGCmd/*, PLGCmd, PWGCmd, QUTCmd,
                                             REVCmd*/};
const size_t UDP_COMMANDS_COUNT = 1;

int handleUDPCommand(const UDPCommandDescriptor *cmd, ServerState *state) {
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
    printf("Response: %s\n", state->out_buffer);

    return HANDLER_SUCCESS;
}

void *fulfillSNGRequest(UNUSED void *req, UNUSED ServerState *state) {
    // SNGMessage *sng = (SNGMessage *)req;
    RSGMessage *rsg = (RSGMessage *)malloc(sizeof(RSGMessage));
    if (rsg == NULL) {
        return NULL;
    }
    printf("UAU\n");
    rsg->status = RSG_NOK;

    return rsg;
}