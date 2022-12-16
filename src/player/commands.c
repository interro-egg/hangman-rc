#include "commands.h"
#include "../common/messages.h"
#include "network.h"
#include "parsers.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *startCmdAliases[] = {"start", "sg"};
const CommandDescriptor startCmd = {startCmdAliases,       2,
                                    parseSNGArgs,          startPreHook,
                                    serializeSNGMessage,   destroySNGMessage,
                                    deserializeRSGMessage, destroyRSGMessage,
                                    startCallback};
char *playCmdAliases[] = {"play", "pl"};
const CommandDescriptor playCmd = {playCmdAliases,        2,
                                   parsePLGArgs,          playPreHook,
                                   serializePLGMessage,   destroyPLGMessage,
                                   deserializeRLGMessage, destroyRLGMessage,
                                   playCallback};
char *guessCmdAliases[] = {"guess", "gw"};
const CommandDescriptor guessCmd = {guessCmdAliases,       2,
                                    parsePWGArgs,          guessPreHook,
                                    serializePWGMessage,   destroyPWGMessage,
                                    deserializeRWGMessage, destroyRWGMessage,
                                    guessCallback};
char *quitCmdAliases[] = {"quit"};
const CommandDescriptor quitCmd = {quitCmdAliases,        1,
                                   parseQUTArgs,          quitPreHook,
                                   serializeQUTMessage,   destroyQUTMessage,
                                   deserializeRQTMessage, destroyRQTMessage,
                                   quitCallback};
char *exitCmdAliases[] = {"exit"};
const CommandDescriptor exitCmd = {exitCmdAliases,        1,
                                   parseQUTArgs,          exitPreHook,
                                   serializeQUTMessage,   destroyQUTMessage,
                                   deserializeRQTMessage, destroyRQTMessage,
                                   exitCallback};

const CommandDescriptor COMMANDS[] = {startCmd, playCmd, guessCmd, quitCmd,
                                      exitCmd};
const size_t COMMANDS_COUNT = 5;

int handleCommand(const CommandDescriptor *cmd, char *args,
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
    printf("Sending message: %s", state->out_buffer);
    errno = 0;
    if (sendUDPMessage(state) == -1) {
        int r = errno == ETIMEDOUT ? HANDLER_ECOMMS_TIMEO : HANDLER_ECOMMS;
        cmd->requestDestroyer(parsed);
        return r;
    }
    printf("Received message: %s", state->in_buffer);
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

void displayWord(char *word) {
    for (size_t i = 0; word[i] != '\0'; i++) {
        printf("%s%c", i != 0 ? " " : "", word[i]);
    }
}

int startPreHook(UNUSED void *req, PlayerState *state) {
    if (state->in_game) {
        printf("You are in a game already. Please finish the game before "
               "starting a new one.\n");
        return -1;
    }
    return 0;
}

void startCallback(void *req, void *resp, UNUSED PlayerState *state) {
    RSGMessage *rsg = (RSGMessage *)resp;
    SNGMessage *sng = (SNGMessage *)req;
    if (rsg->status == RSG_NOK) {
        printf("Game could not be started. Please try again.");
        return;
    }
    state->word = malloc(rsg->n_letters * sizeof(char) + 1);
    for (size_t i = 0; i < rsg->n_letters; i++) {
        state->word[i] = '_';
    }
    state->word[rsg->n_letters] = '\0';
    state->remaining_errors = rsg->remaining_errors;
    state->in_game = true;
    state->trial = 0;
    state->PLID = malloc(7 * sizeof(char));
    strcpy(state->PLID, sng->PLID);
    printf("New game started. Guess %u letter word: ", rsg->n_letters);
    displayWord(state->word);
    printf("\n");
    printf("You have %u guesses left.\n", state->remaining_errors);
}

int playPreHook(void *req, PlayerState *state) {
    if (!state->in_game) {
        printf("You are not in a game. Please start a new one.\n");
        return -1;
    }
    PLGMessage *plg = (PLGMessage *)req;
    strcpy(plg->PLID, state->PLID);
    plg->trial = ++state->trial; // TODO: does this work?
    return 0;
}

void playCallback(void *req, void *resp, PlayerState *state) {
    PLGMessage *plg = (PLGMessage *)req;
    RLGMessage *rlg = (RLGMessage *)resp;
    if (rlg->status == RLG_OK) {
        printf("Letter %c is in the word.\n", plg->letter);
        printf("Number of letters: %u\n", rlg->n);
        for (unsigned int i = 0; i < rlg->n; i++) {
            printf("Position: %u\n", rlg->pos[i]);
            state->word[rlg->pos[i] - 1] =
                plg->letter; // FIXME: this doesn't seem to work
        }
        printf("Correct!\n");
        displayWord(state->word);
    } else if (rlg->status == RLG_WIN) {
        printf("You won! Congratulations!\n");
        state->in_game = false;
        free(state->word);
        state->word = NULL;
        return;
    } else if (rlg->status == RLG_DUP) {
        printf("You have already guessed this letter. Please try again.\n");
        displayWord(state->word);
        state->trial--;
    } else if (rlg->status == RLG_NOK) {
        state->remaining_errors--;
        printf("Wrong letter. Please try again.\n");
        displayWord(state->word);
    } else if (rlg->status == RLG_OVR) {
        printf("You have exceeded the maximum number of errors. You lost!\n");
        state->in_game = false;
        free(state->word);
        state->word = NULL;
        return;
    } else if (rlg->status == RLG_INV) {
        printf("An error occurred. Please try again.\n");
        displayWord(state->word);
    } else if (rlg->status == RLG_ERR) {
        printf("An error occurred. Please try again.\n");
        displayWord(state->word);
        state->trial--;
    }
    printf("\n");
    printf("You have %u guesses left.\n", state->remaining_errors);
}

int guessPreHook(void *req, PlayerState *state) {
    if (!state->in_game) {
        printf("You are not in a game. Please start a new one.\n");
        return -1;
    }
    PWGMessage *pwg = (PWGMessage *)req;
    strcpy(pwg->PLID, state->PLID);
    pwg->trial = ++state->trial; // TODO: does this work?
    return 0;
}

void guessCallback(UNUSED void *req, void *resp, PlayerState *state) {
    PWGMessage *pwg = (PWGMessage *)req;
    RWGMessage *rwg = (RWGMessage *)resp;
    if (rwg->status == RWG_WIN) {
        strcpy(state->word, pwg->word);
        displayWord(state->word);
        printf("\nYou won! Congratulations!\n");
        state->in_game = false;
        free(state->word);
        state->word = NULL;
    } else if (rwg->status == RWG_NOK) {
        printf("Wrong word. Please try again.\n");
        displayWord(state->word);
        printf("\n");
    } else if (rwg->status == RWG_OVR) {
        printf("You have exceeded the maximum number of errors. You lost!\n");
        state->in_game = false;
        free(state->word);
        state->word = NULL;
    } else if (rwg->status == RWG_INV) {
        printf("An error occurred. Please try again.\n");
        displayWord(state->word);
        printf("\n");
    } else if (rwg->status == RWG_ERR) {
        printf("An error occurred. Please try again.\n");
        displayWord(state->word);
        printf("\n");
        state->trial--;
    }
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
        state->in_game = false;
        free(state->word);
        state->word = NULL;
    } else if (rqt->status == RQT_ERR) {
        printf("An error occurred. Please try again.\n");
    }
}

int exitPreHook(void *req, PlayerState *state) {
    if (!state->in_game) {
        exit(0); // FIXME: this is not a good way to do this
    }
    QUTMessage *qut = (QUTMessage *)req;
    strcpy(qut->PLID, state->PLID);
    return 0;
}

void exitCallback(UNUSED void *req, UNUSED void *resp,
                  UNUSED PlayerState *state) {
    exit(0); // FIXME: this is not a good way to do this
}