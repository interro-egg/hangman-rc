#include "commands.h"
#include "../common/messages.h"
#include "network.h"
#include "parsers.h"
#include <errno.h>
#include <stdio.h>

char *startCmdAliases[] = {"start", "sg"};
const CommandDescriptor startCmd = {startCmdAliases,   2,
                                    parseSNGArgs,      serializeSNGMessage,
                                    destroySNGMessage, deserializeRSGMessage,
                                    destroyRSGMessage, startCallback};

const CommandDescriptor COMMANDS[] = {startCmd};
const size_t COMMANDS_COUNT = 1;

int handleCommand(const CommandDescriptor *cmd, char *args,
                  PlayerState *state) {
    void *parsed = cmd->argsParser(args);
    if (parsed == NULL) {
        return (errno == ENOMEM) ? HANDLER_ENOMEM : HANDLER_EPARSE;
    }
    ssize_t req = cmd->requestSerializer(parsed, state->out_buffer);
    if (req < 0) {
        cmd->requestDestroyer(parsed);
        return HANDLER_ESERIALIZE;
    }
    if (sendUDPMessage(state) == -1) {
        cmd->requestDestroyer(parsed);
        return HANDLER_ECOMMS;
    }
    void *deserialized = cmd->responseDeserializer(state->in_buffer);
    if (deserialized == NULL) {
        cmd->requestDestroyer(parsed);
        cmd->responseDestroyer(deserialized);
        return HANDLER_EDESERIALIZE;
    }

    int result = cmd->callback(parsed, deserialized, state);

    cmd->requestDestroyer(parsed);
    cmd->responseDestroyer(deserialized);
    return result == 0 ? HANDLER_SUCCESS : HANDLER_EUNKNOWN;
}

int startCallback(void *req, void *resp, UNUSED PlayerState *state) {
    UNUSED SNGMessage *sng = (SNGMessage *)req;
    UNUSED RSGMessage *rsg = (RSGMessage *)resp;
    // TODO: stuff
    printf("test %s\n", RSGMessageStatusStrings[rsg->status]);
    return 0;
}