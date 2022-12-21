#include "commands.h"
#include "../common/common.h"
#include "network.h"
#include "persistence.h"
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

const UDPCommandDescriptor PWGCmd = {"PWG",
                                     deserializePWGMessage,
                                     destroyPWGMessage,
                                     fulfillPWGRequest,
                                     serializeRWGMessage,
                                     destroyRWGMessage,
                                     "RWG"};

const UDPCommandDescriptor QUTCmd = {"QUT",
                                     deserializeQUTMessage,
                                     destroyQUTMessage,
                                     fulfillQUTRequest,
                                     serializeRQTMessage,
                                     destroyRQTMessage,
                                     "RQT"};

const UDPCommandDescriptor REVCmd = {"REV",
                                     deserializeREVMessage,
                                     destroyREVMessage,
                                     fulfillREVRequest,
                                     serializeRRVMessage,
                                     destroyRRVMessage,
                                     "RRV"};

const UDPCommandDescriptor UDP_COMMANDS[] = {SNGCmd, PLGCmd, PWGCmd, QUTCmd,
                                             REVCmd};
const size_t UDP_COMMANDS_COUNT = 5;

const TCPCommandDescriptor GSBCmd = {"GSB",
                                     deserializeGSBMessage,
                                     destroyGSBMessage,
                                     fulfillGSBRequest,
                                     RSBMessageStatusStrings,
                                     RSBMessageFileReceiveStatuses,
                                     "RSB"};

const TCPCommandDescriptor GHLCmd = {"GHL",
                                     deserializeGHLMessage,
                                     destroyGHLMessage,
                                     fulfillGHLRequest,
                                     RHLMessageStatusStrings,
                                     RHLMessageFileReceiveStatuses,
                                     "RHL"};

const TCPCommandDescriptor STACmd = {"STA",
                                     deserializeSTAMessage,
                                     destroySTAMessage,
                                     fulfillSTARequest,
                                     RSTMessageStatusStrings,
                                     RSTMessageFileReceiveStatuses,
                                     "RST"};

const TCPCommandDescriptor TCP_COMMANDS[] = {GSBCmd, GHLCmd, STACmd};
const size_t TCP_COMMANDS_COUNT = 3;

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

    r = replyTCP(file, state) == 0 ? HANDLER_SUCCESS : HANDLER_ECOMMS;
    destroyResponseFile(file);
    return r;
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
        if (game == NULL || saveGame(game) != 0) {
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

    if (plg->trial != game->numTrials + 1 && plg->trial != game->numTrials) {
        // If it's not repetition of last trial or current trial
        rlg->status = RLG_INV;
    } else if (plg->trial == game->numTrials) {
        // If it's a repetition of last trial
        if (plg->letter != game->trials[plg->trial - 1]->guess.letter) {
            // If letter is different now
            rlg->status = RLG_INV;
        } else {
            // check if letter has been already guessed up until last trial
            for (size_t i = 0; i < game->numTrials - 1; i++) {
                if (game->trials[i]->type == TRIAL_TYPE_LETTER &&
                    game->trials[i]->guess.letter == plg->letter) {
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
            if (game->trials[i]->type == TRIAL_TYPE_LETTER &&
                game->trials[i]->guess.letter == plg->letter) {
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

            if (game->numTrials - game->numSucc >= game->maxErrors) {
                // It's Game Over
                rlg->status = RLG_OVR;
            }

        } else if (game->remainingLetters - rlg->n == 0) {
            // It's a win
            game->numSucc++;
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

    if (rlg->status == RLG_OVR || rlg->status == RLG_WIN) {
        if (endGame(game,
                    rlg->status == RLG_WIN ? OUTCOME_WIN : OUTCOME_FAIL) != 0) {
            return NULL;
        }
    } else if (saveGame(game) != 0) {
        return NULL;
    }
    return rlg;
}

void *fulfillPWGRequest(void *req, UNUSED ServerState *state) {
    PWGMessage *pwg = (PWGMessage *)req;
    RWGMessage *rwg = (RWGMessage *)malloc(sizeof(RWGMessage));
    if (rwg == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    Game *game = loadGame(pwg->PLID, true);
    if (game == NULL) {
        return NULL;
    }
    // If it's not repetition of last trial or current trial
    if (pwg->trial != game->numTrials + 1 && pwg->trial != game->numTrials) {
        rwg->status = RWG_INV;
    }
    // If it's a repetition of last trial
    else if (pwg->trial == game->numTrials) {
        // If word is different now
        if (strcmp(pwg->word, game->trials[game->numTrials - 1]->guess.word) !=
            0) {
            rwg->status = RWG_INV;
        }
        // check if word has been already guessed up until last trial
        else {
            for (size_t i = 0; i < game->numTrials - 1; i++) {
                if (game->trials[i]->type == TRIAL_TYPE_WORD &&
                    strcmp(game->trials[i]->guess.word, pwg->word) == 0) {
                    rwg->status = RWG_DUP;
                    rwg->trials = game->numTrials;
                    return rwg;
                }
            }
            // No need to test/set anything else because it's a repetition
            rwg->status = RWG_NOK;
        }

    } else {
        // It's a new trial
        for (size_t i = 0; i < game->numTrials; i++) {
            if (game->trials[i]->type == TRIAL_TYPE_WORD &&
                strcmp(game->trials[i]->guess.word, pwg->word) == 0) {
                rwg->status = RWG_DUP;
                rwg->trials = game->numTrials;
                return rwg;
            }
        }
        if (strcmp(pwg->word, game->wordListEntry->word) != 0) {
            rwg->status = RWG_NOK;
            // check if it's a gameover
            if (game->numTrials - game->numSucc >= game->maxErrors) {
                rwg->status = RWG_OVR;
            }
        } else {
            game->numSucc++;
            rwg->status = RWG_WIN;
        }
        GameTrial *newTrial = malloc(sizeof(GameTrial));
        if (newTrial == NULL) {
            return NULL;
        }
        newTrial->type = TRIAL_TYPE_WORD;
        newTrial->guess.word = pwg->word;
        if (registerGameTrial(game, newTrial) != 0) {
            return NULL;
        }
    }
    rwg->trials = game->numTrials;

    if (rwg->status == RWG_OVR || rwg->status == RWG_WIN) {
        if (endGame(game,
                    rwg->status == RWG_WIN ? OUTCOME_WIN : OUTCOME_FAIL) != 0) {
            return NULL;
        }
    } else if (saveGame(game) != 0) {
        return NULL;
    }
    return rwg;
}

void *fulfillQUTRequest(void *req, UNUSED ServerState *state) {
    QUTMessage *qut = (QUTMessage *)req;
    RQTMessage *rqt = (RQTMessage *)malloc(sizeof(RQTMessage));
    if (rqt == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    Game *game = loadGame(qut->PLID, true);
    if (game == NULL) {
        rqt->status = RQT_NOK;
    } else {
        if (endGame(game, OUTCOME_QUIT) != 0) {
            return NULL;
        }
        rqt->status = RQT_OK;
    }
    return rqt;
}

void *fulfillREVRequest(void *req, UNUSED ServerState *state) {
    REVMessage *rev = (REVMessage *)req;
    RRVMessage *rrv = (RRVMessage *)malloc(sizeof(RRVMessage));
    if (rrv == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    Game *game = loadGame(rev->PLID, true);
    if (game == NULL) {
        return NULL;
    } else {
        rrv->type = RRV_WORD;
        rrv->data.word = game->wordListEntry->word;
    }
    return rrv;
}

int fulfillGSBRequest(UNUSED void *req, UNUSED ServerState *state,
                      ResponseFile **fptr) {
    *fptr = getScoreboard();

    return (*fptr != NULL) ? RSB_OK : RSB_EMPTY;
}

int fulfillGHLRequest(void *req, UNUSED ServerState *state,
                      ResponseFile **fptr) {
    GHLMessage *ghl = (GHLMessage *)req;
    Game *game = loadGame(ghl->PLID, true);
    if (game == NULL) {
        return RHL_NOK;
    }

    *fptr = getFSResponseFile(HINTS_DIR, game->wordListEntry->hintFile, NULL);

    return (*fptr != NULL) ? RHL_OK : RHL_NOK;
}

int fulfillSTARequest(void *req, UNUSED ServerState *state,
                      ResponseFile **fptr) {
    STAMessage *sta = (STAMessage *)req;
    Game *game = loadGame(sta->PLID, false);
    if (game == NULL) {
        return RST_NOK;
    }

    *fptr = getGameState(game);

    if (*fptr == NULL) {
        return RST_NOK;
    } else if (game->outcome == OUTCOME_ONGOING) {
        return RST_ACT;
    } else {
        return RST_FIN;
    }
}