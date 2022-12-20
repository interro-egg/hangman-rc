#include "commands.h"
#include "../common/common.h"
#include "persistence.h"
#include "network.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    errno = 0;
    void *resp = cmd->requestFulfiller(req, state);
    if (resp == NULL) {
        int r = errno == ENOMEM ? HANDLER_ENOMEM : HANDLER_EFULFILL;
        cmd->requestDestroyer(req);
        return r;
    }

    if (cmd->responseSerializer(resp, state->out_buffer) <= 0) {
        cmd->requestDestroyer(req);
        cmd->responseDestroyer(resp);
        return HANDLER_ESERIALIZE;
    }

    if (replyUDP(state) == -1) {
        cmd->requestDestroyer(req);
        cmd->responseDestroyer(resp);
        return HANDLER_ECOMMS;
    }

    return HANDLER_SUCCESS;
}

void *fulfillSNGRequest(void *req, ServerState *state) {
    SNGMessage *sng = (SNGMessage *)req;
    RSGMessage *rsg = (RSGMessage *)malloc(sizeof(RSGMessage));
    if (rsg == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    Game *game = loadGame(sng->PLID, true);
    if (game != NULL && game->numTrials != 0) {
        rsg->status = RSG_NOK;
        rsg->n_letters = 0;
        rsg->max_errors = 0;
        return rsg;
    }
    if (game == NULL) {
        game = newGame(sng->PLID, state);
        if (game == NULL) {
            return NULL;
        }
        if (saveGame(game, state) != 0) {
            return NULL;
        }
    }
    rsg->status = RSG_OK;
    rsg->n_letters = (unsigned int)strlen(game->wordListEntry->word);
    rsg->max_errors = game->maxErrors;
    return rsg;
}

// void *fulfillPLGRequest(UNUSED void *req, UNUSED ServerState *state) {
//     PLGMessage *plg = (PLGMessage *)req;
//     RLGMessage *rlg = (RLGMessage *)malloc(sizeof(RLGMessage));
//     if (rlg == NULL) {
//         errno = ENOMEM;
//         return NULL;
//     }
//     Game *game = loadGame(plg->PLID, false);
//     if (game == NULL) {
//         rlg->status = RLG_NOK;
//         return rlg;
//     }
//     if (game->numTrials == 0) {
//         rlg->status = RLG_OVR;
//         return rlg;
//     }
//     if (plg->trial != game->numTrials) {
//         rlg->status = RLG_INV;
//         return rlg;
//     }
//     if (game->numErrors >= game->maxErrors) {
//         rlg->status = RLG_OVR;
//         return rlg;
//     }
//     if (game->wordListEntry->word[plg->trial] == plg->letter) {
//         rlg->status = RLG_WIN;
//         return rlg;
//     }
//     if (game->wordListEntry->word[plg->trial] != plg->letter) {
//         rlg->status = RLG_NOK;
//         return rlg;
//     }
//     return rlg;
// }