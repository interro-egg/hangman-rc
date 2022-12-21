#include "commands.h"
#include "../common/common.h"
#include "network.h"
#include "persistence.h"
#include "server_state.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const UDPCommandDescriptor SNGCmd = {"SNG",
                                     deserializeSNGMessage,
                                     destroySNGMessage,
                                     fulfillSNGRequest,
                                     serializeRSGMessage,
                                     destroyRSGMessage,
                                     "RSG"};

const UDPCommandDescriptor PLGCmd = {"PLG",
                                     deserializePLGMessage,
                                     destroyPLGMessage,
                                     fulfillPLGRequest,
                                     serializeRLGMessage,
                                     destroyRLGMessage,
                                     "RLG"};

/*
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
const UDPCommandDescriptor UDP_COMMANDS[] = {SNGCmd, PLGCmd/*, PWGCmd, QUTCmd,
                                             REVCmd*/};
const size_t UDP_COMMANDS_COUNT = 2;

const TCPCommandDescriptor GSBCmd = {"GSB",
                                     deserializeGSBMessage,
                                     destroyGSBMessage,
                                     fulfillGSBRequest,
                                     RSBMessageStatusStrings,
                                     RSBMessageFileReceiveStatuses,
                                     "RSB"};

const TCPCommandDescriptor TCP_COMMANDS[] = {GSBCmd};
const size_t TCP_COMMANDS_COUNT = 1;

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

int handleTCPCommand(const TCPCommandDescriptor *cmd, ServerState *state) {
    void *req = cmd->requestDeserializer(state->in_buffer);
    if (req == NULL) {
        return HANDLER_EDESERIALIZE;
    }

    ResponseFile *file;

    errno = 0;
    int status = cmd->requestFulfiller(req, state, &file);
    int r = errno == ENOMEM ? HANDLER_ENOMEM : HANDLER_EFULFILL;
    cmd->requestDestroyer(req);
    if (status < 0) {
        return r;
    }

    if (!cmd->fileSendStatuses[status]) {
        file = NULL;
    } else if (file == NULL) {
        return HANDLER_EFILE_REQUIRED;
    }

    if (sprintf(state->out_buffer, "%s %s", cmd->response,
                cmd->statusEnumStrings[status]) <= 0) {
        return HANDLER_ESERIALIZE;
    }

    return replyTCP(file, state) == 0 ? HANDLER_SUCCESS : HANDLER_ECOMMS;
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
        return rsg;
    }
    if (game == NULL) {
        game = newGame(sng->PLID, state);
        if (game == NULL) {
            return NULL;
        }
        if (saveGame(game) != 0) {
            return NULL;
        }
    }
    rsg->status = RSG_OK;
    rsg->n_letters = (unsigned int)strlen(game->wordListEntry->word);
    rsg->max_errors = game->maxErrors;
    return rsg;
}

void *fulfillPLGRequest(void *req, UNUSED ServerState *state) {
    PLGMessage *plg = (PLGMessage *)req;
    RLGMessage *rlg = (RLGMessage *)malloc(sizeof(RLGMessage));
    rlg->pos = NULL;
    if (rlg == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    Game *game = loadGame(plg->PLID, true);
    if (game == NULL) {
        return NULL;
    }
    // If it's not repetition of last trial or current trial
    if (plg->trial != game->numTrials + 1 && plg->trial != game->numTrials) {
        rlg->status = RLG_INV;
    }
    // If it's a repetition of last trial
    else if (plg->trial == game->numTrials) {
        // If letter is different now
        if (plg->letter != game->trials[plg->trial - 1]->guess.letter) {
            rlg->status = RLG_INV;
        }
        // check if letter has been already guessed up until last trial
        else {
            for (size_t i = 0; i < game->numTrials - 1; i++) {
                if (game->trials[i]->guess.letter == plg->letter) {
                    rlg->status = RLG_DUP;
                    rlg->trial = game->numTrials;
                    return rlg;
                }
            }
            errno = 0;
            rlg->n = getLetterPositions(plg->letter, game->wordListEntry->word,
                                        &rlg->pos);
            if (rlg->n == 0 && errno == ENOMEM) {
                return NULL;
            }
            if (rlg->n == 0) {
                rlg->status = RLG_NOK;
            } else {
                rlg->status = RLG_OK;
            }
            // No need to test/set anything else because it's a repetition
        }

    } else {
        // It's a new trial
        for (size_t i = 0; i < game->numTrials; i++) {
            if (game->trials[i]->guess.letter == plg->letter) {
                rlg->status = RLG_DUP;
                rlg->trial = game->numTrials;
                return rlg;
            }
        }
        errno = 0;
        rlg->n = getLetterPositions(plg->letter, game->wordListEntry->word,
                                    &rlg->pos);

        if (rlg->n == 0 && errno == ENOMEM) {
            return NULL;
        }
        if (rlg->n == 0) {
            rlg->status = RLG_NOK;

            // check if it's a gameover
            if (game->numTrials - game->numSucc >= game->maxErrors) {
                rlg->status = RLG_OVR;
            }

        } // check if it's a win
        else if (game->remainingLetters - rlg->n == 0) {
            rlg->status = RLG_WIN;
        } else {
            game->remainingLetters -= rlg->n;
            game->numSucc++;
            rlg->status = RLG_OK;
        }
        GameTrial *newTrial = malloc(sizeof(GameTrial));
        if (newTrial == NULL) {
            return NULL;
        }
        newTrial->type = TRIAL_TYPE_LETTER;
        newTrial->guess.letter = plg->letter;
        if (registerGameTrial(game, newTrial) != 0) {
            return NULL;
        }
    }
    // n and pos are already set
    rlg->trial = game->numTrials;
    if (saveGame(game) != 0) {
        return NULL;
    }
    return rlg;
}

int fulfillGSBRequest(UNUSED void *req, UNUSED ServerState *state,
                      ResponseFile **fptr) {
    ResponseFile *file = malloc(sizeof(ResponseFile));
    if (file == NULL) {
        return -1;
    }

    // TODO: implement

    file->name = "test.txt";
    file->size = 4;
    file->data = "abcd";
    *fptr = file;

    return RSB_OK;

    return 0;
}