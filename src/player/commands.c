#include "commands.h"
#include "../common/messages.h"
#include "network.h"
#include "parsers.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *startCmdAliases[] = {"start", "sg"};
const UDPCommandDescriptor startCmd = {startCmdAliases,       2,
                                       parseSNGArgs,          startPreHook,
                                       serializeSNGMessage,   destroySNGMessage,
                                       deserializeRSGMessage, destroyRSGMessage,
                                       startCallback};
char *playCmdAliases[] = {"play", "pl"};
const UDPCommandDescriptor playCmd = {playCmdAliases,        2,
                                      parsePLGArgs,          playPreHook,
                                      serializePLGMessage,   destroyPLGMessage,
                                      deserializeRLGMessage, destroyRLGMessage,
                                      playCallback};
char *guessCmdAliases[] = {"guess", "gw"};
const UDPCommandDescriptor guessCmd = {guessCmdAliases,       2,
                                       parsePWGArgs,          guessPreHook,
                                       serializePWGMessage,   destroyPWGMessage,
                                       deserializeRWGMessage, destroyRWGMessage,
                                       guessCallback};
char *quitCmdAliases[] = {"quit"};
const UDPCommandDescriptor quitCmd = {quitCmdAliases,        1,
                                      parseQUTArgs,          quitPreHook,
                                      serializeQUTMessage,   destroyQUTMessage,
                                      deserializeRQTMessage, destroyRQTMessage,
                                      quitCallback};
char *exitCmdAliases[] = {"exit"};
const UDPCommandDescriptor exitCmd = {exitCmdAliases,        1,
                                      parseQUTArgs,          exitPreHook,
                                      serializeQUTMessage,   destroyQUTMessage,
                                      deserializeRQTMessage, destroyRQTMessage,
                                      exitCallback};

const UDPCommandDescriptor UDP_COMMANDS[] = {startCmd, playCmd, guessCmd,
                                             quitCmd, exitCmd};
const size_t UDP_COMMANDS_COUNT = 5;

int handleUDPCommand(const UDPCommandDescriptor *cmd, char *args,
                     PlayerState *state) {
    printf("Handling command %s\n", cmd->aliases[0]);
    void *parsed = cmd->argsParser(args);
    if (parsed == NULL) {

        return (errno == ENOMEM) ? HANDLER_ENOMEM : HANDLER_EPARSE;
    }
    if (cmd->preHook != NULL && cmd->preHook(parsed, state) != 0) {
        cmd->requestDestroyer(parsed);
        return HANDLER_SUCCESS; // don't show message
    }
    ssize_t req = cmd->requestSerializer(parsed, state->out_buffer);
    if (req < 0) {
        cmd->requestDestroyer(parsed);
        return HANDLER_ESERIALIZE;
    }
    printf("Sending message: |%s|", state->out_buffer);
    errno = 0;
    if (sendUDPMessage(state) == -1) {
        int r = errno == ETIMEDOUT ? HANDLER_ECOMMS_TIMEO : HANDLER_ECOMMS;
        cmd->requestDestroyer(parsed);
        return r;
    }
    printf("Received message: |%s|", state->in_buffer);
    void *deserialized = cmd->responseDeserializer(state->in_buffer);
    if (deserialized == NULL) {
        cmd->requestDestroyer(parsed);
        cmd->responseDestroyer(deserialized);
        return HANDLER_EDESERIALIZE;
    }
    if (cmd->callback != NULL) {
        cmd->callback(parsed, deserialized, state);
    }

    cmd->requestDestroyer(parsed);
    cmd->responseDestroyer(deserialized);
    return HANDLER_SUCCESS;
}

int startPreHook(UNUSED void *req, PlayerState *state) {
    if (state->in_game) {
        printf("You are in a game already. Please finish the game before "
               "starting a new one.\n");
        return -1;
    }
    return 0;
}

void startCallback(void *req, void *resp, PlayerState *state) {
    SNGMessage *sng = (SNGMessage *)req;
    RSGMessage *rsg = (RSGMessage *)resp;
    if (rsg->status == RSG_NOK) {
        printf("A game could not be started. A game might already be on-course "
               "for this PLID.\n");
        return;
    } else if (rsg->status == RSG_ERR) {
        printf("A game could not be started. You might have specified an "
               "invalid PLID; please try again.\n");
        return;
    }
    if (startGame(state, rsg->n_letters, rsg->max_errors, sng->PLID) == -1) {
        printf("Failed to start game. Please try again.");
    }
    printf("New game started. Guess %u letter word: ", rsg->n_letters);
    displayWord(state->word);
    printf("\nYou have %u wrong guesses left.\n", state->remaining_errors);
}

int playPreHook(void *req, PlayerState *state) {
    if (!state->in_game) {
        printf("You are not in a game. Please start a new one.\n");
        return -1;
    }
    PLGMessage *plg = (PLGMessage *)req;
    strcpy(plg->PLID, state->PLID);
    plg->trial = ++state->trial;
    return 0;
}

void playCallback(void *req, void *resp, PlayerState *state) {
    PLGMessage *plg = (PLGMessage *)req;
    RLGMessage *rlg = (RLGMessage *)resp;
    if (rlg->status == RLG_OK) {
        for (size_t i = 0; i < rlg->n; i++) {
            state->word[rlg->pos[i] - 1] = toupper(plg->letter);
        }
        printf("Correct!\n");
    } else if (rlg->status == RLG_WIN) {
        printf("You won! Congratulations!\n");
        endGame(state);
        return;
    } else if (rlg->status == RLG_DUP) {
        printf("You have already guessed this letter. Please try again.\n");
        state->trial--;
    } else if (rlg->status == RLG_NOK) {
        printf("Wrong letter. Please try again.\n");
        state->remaining_errors--;
    } else if (rlg->status == RLG_OVR) {
        printf("Wrong letter. You have exceeded the maximum number of errors. "
               "You lost!\n");
        endGame(state);
        return;
    } else if (rlg->status == RLG_INV) {
        printf("An error occurred. Please try again.\n");
    } else if (rlg->status == RLG_ERR) {
        printf("An error occurred. Please try again.\n");
        state->trial--;
    }
    displayWord(state->word);
    printf("\nYou have %u wrong guesses left.\n", state->remaining_errors);
}

int guessPreHook(void *req, PlayerState *state) {
    if (!state->in_game) {
        printf("You are not in a game. Please start a new one.\n");
        return -1;
    }
    PWGMessage *pwg = (PWGMessage *)req;
    strcpy(pwg->PLID, state->PLID);
    pwg->trial = ++state->trial;
    return 0;
}

void guessCallback(UNUSED void *req, void *resp, PlayerState *state) {
    PWGMessage *pwg = (PWGMessage *)req;
    RWGMessage *rwg = (RWGMessage *)resp;
    if (rwg->status == RWG_WIN) {
        strcpy(state->word, pwg->word);
        displayWord(state->word);
        printf("\nYou won! Congratulations!\n");
        endGame(state);
        return;
    } else if (rwg->status == RWG_DUP) {
        printf("You have already guessed this word. Please try again.\n");
        state->trial--;
    } else if (rwg->status == RWG_NOK) {
        printf("Wrong word. Please try again.\n");
        state->remaining_errors--;
    } else if (rwg->status == RWG_OVR) {
        printf("Wrong word. You have exceeded the maximum number of errors. "
               "You lost!\n");
        endGame(state);
        return;
    } else if (rwg->status == RWG_INV) {
        printf("An error occurred. Please try again.\n");
    } else if (rwg->status == RWG_ERR) {
        printf("An error occurred. Please try again.\n");
        state->trial--;
    }
    displayWord(state->word);
    printf("\n");
    printf("\nYou have %u wrong guesses left.\n", state->remaining_errors);
}

int quitPreHook(void *req, PlayerState *state) {
    if (!state->in_game) {
        printf("You are not in a game. Please start a new one.\n");
        return -1;
    }
    QUTMessage *qut = (QUTMessage *)req;
    strcpy(qut->PLID, state->PLID);
    return 0;
}

void quitCallback(UNUSED void *req, void *resp, PlayerState *state) {
    RQTMessage *rqt = (RQTMessage *)resp;
    if (rqt->status == RQT_OK) {
        printf("You have quit the game. Please start a new one.\n");
        endGame(state);
    } else if (rqt->status == RQT_NOK) {
        printf("You do not have a game to quit. Please start a new one.\n");
    } else if (rqt->status == RQT_ERR) {
        printf("An error occurred. Please try again.\n");
    }
}

int exitPreHook(void *req, PlayerState *state) {
    if (!state->in_game) {
        exit(0); // FIXME: this is not a good way to do this
    }
    QUTMessage *qut = (QUTMessage *)req;
    strncpy(qut->PLID, state->PLID, 6 + 1);
    return 0;
}

void exitCallback(UNUSED void *req, UNUSED void *resp,
                  UNUSED PlayerState *state) {
    exit(0); // FIXME: this is not a good way to do this
}