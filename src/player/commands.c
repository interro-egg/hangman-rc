#include "commands.h"
#include "../common/messages.h"
#include "network.h"
#include "parsers.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

char *revealCmdAliases[] = {"reveal", "rev"};
const UDPCommandDescriptor revealCmd = {
    revealCmdAliases,      2,
    parseREVArgs,          revealPreHook,
    serializeREVMessage,   destroyREVMessage,
    deserializeRRVMessage, destroyRRVMessage,
    revealCallback};

const UDPCommandDescriptor UDP_COMMANDS[] = {startCmd, playCmd, guessCmd,
                                             quitCmd,  exitCmd, revealCmd};
const size_t UDP_COMMANDS_COUNT = 6;

char *scoreboardCmdAliases[] = {"scoreboard", "sb"};
const TCPCommandDescriptor scoreboardCmd = {scoreboardCmdAliases,
                                            2,
                                            parseGSBArgs,
                                            NULL,
                                            serializeGSBMessage,
                                            destroyGSBMessage,
                                            "RSB",
                                            RSBMessageStatusStrings,
                                            5,
                                            RSBMessageFileReceiveStatuses,
                                            scoreboardCallback};

const TCPCommandDescriptor TCP_COMMANDS[] = {scoreboardCmd};
const size_t TCP_COMMANDS_COUNT = 1;

int handleUDPCommand(const UDPCommandDescriptor *cmd, char *args,
                     PlayerState *state) {
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

    errno = 0;
    if (sendUDPMessage(state) == -1) {
        int r = errno == ETIMEDOUT ? HANDLER_ECOMMS_TIMEO : HANDLER_ECOMMS_UDP;
        cmd->requestDestroyer(parsed);
        return r;
    }

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

int handleTCPCommand(const TCPCommandDescriptor *cmd, char *args,
                     PlayerState *state) {
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
    int fd = sendTCPMessage(state);
    if (fd < 0) {
        errno = fd;
        cmd->requestDestroyer(parsed);
        return HANDLER_ECOMMS_TCP;
    }

    ssize_t r = readWordTCP(fd, state->in_buffer, MESSAGE_COMMAND_SIZE, false);
    if (r <= 0) {
        errno = (int)r;
        cmd->requestDestroyer(parsed);
        return HANDLER_ECOMMS_TCP;
    }
    if (strcmp(state->in_buffer, cmd->expectedResponse) != 0) {
        cmd->requestDestroyer(parsed);
        return HANDLER_EDESERIALIZE;
    }

    r = readWordTCP(fd, state->in_buffer, cmd->maxStatusEnumLen, false);
    if (r <= 0) {
        errno = (int)r;
        cmd->requestDestroyer(parsed);
        return HANDLER_ECOMMS_TCP;
    }

    int status = parseEnum(cmd->statusEnumStrings, state->in_buffer);
    if (status == -1) {
        cmd->requestDestroyer(parsed);
        return HANDLER_EDESERIALIZE;
    }

    char *fname = NULL;
    if (cmd->fileReceiveStatuses[status]) {
        errno = 0;
        if ((fname = readFileTCP(fd)) == NULL) {
            cmd->requestDestroyer(parsed);
            return errno == TCP_RCV_ENOMEM ? HANDLER_ENOMEM
                                           : HANDLER_ECOMMS_TCP;
        }
    }

    if (cmd->callback != NULL) {
        cmd->callback(parsed, status, fname, state);
    }

    cmd->requestDestroyer(parsed);
    free(fname);
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
    strncpy(plg->PLID, state->PLID, 6 + 1);
    plg->trial = ++state->trial;
    return 0;
}

void playCallback(void *req, void *resp, PlayerState *state) {
    PLGMessage *plg = (PLGMessage *)req;
    RLGMessage *rlg = (RLGMessage *)resp;
    if (rlg->status == RLG_OK) {
        for (size_t i = 0; i < rlg->n; i++) {
            state->word[rlg->pos[i] - 1] = (char)toupper(plg->letter);
        }
        printf("Correct!\n");
    } else if (rlg->status == RLG_WIN) {
        for (size_t i = 0; i < strlen(state->word); i++) {
            if (state->word[i] == '_') {
                state->word[i] = (char)toupper(plg->letter);
            }
        }
        printf("Correct!\n");
        displayWord(state->word);
        printf("\nYou won! Congratulations!\n");
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
    strncpy(pwg->PLID, state->PLID, 6 + 1);
    pwg->trial = ++state->trial;
    return 0;
}

void guessCallback(void *req, void *resp, PlayerState *state) {
    PWGMessage *pwg = (PWGMessage *)req;
    RWGMessage *rwg = (RWGMessage *)resp;
    if (rwg->status == RWG_WIN) {
        strncpy(state->word, pwg->word, strlen(state->word) + 1);
        for (size_t i = 0; i < strlen(state->word); i++) {
            state->word[i] = (char)toupper(state->word[i]);
        }
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
    strncpy(qut->PLID, state->PLID, 6 + 1);
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
        state->shutdown = true;
        return -1;
    }
    QUTMessage *qut = (QUTMessage *)req;
    strncpy(qut->PLID, state->PLID, 6 + 1);
    return 0;
}

void exitCallback(UNUSED void *req, UNUSED void *resp,
                  UNUSED PlayerState *state) {
    state->shutdown = true;
}

int revealPreHook(void *req, PlayerState *state) {
    if (!state->in_game) {
        printf("You are not in a game. Please start a new one.\n");
        return -1;
    }
    REVMessage *rev = (REVMessage *)req;
    strncpy(rev->PLID, state->PLID, 6 + 1);
    return 0;
}

void revealCallback(UNUSED void *req, void *resp, UNUSED PlayerState *state) {
    RRVMessage *rrv = (RRVMessage *)resp;
    if (rrv->type == RRV_WORD) {
        printf("The word is: ");
        displayWord(rrv->word);
        putchar('\n');
    } else if (rrv->type == RRV_STATUS) {
        if (rrv->status == RRV_OK) {
            printf("This feature is not currently enabled.\n");
        } else if (rrv->status == RRV_ERR) {
            printf("An error occurred. Please try again.\n");
        }
    }
}

void scoreboardCallback(UNUSED void *req, int status, char *fname,
                        UNUSED PlayerState *state) {
    if (status == RSB_OK) {
        FILE *f = fopen(fname, "r");
        if (f == NULL) {
            printf("Failed to display scoreboard.\n");
        } else {
            char buf[FILE_TRANSFER_BLOCK_SIZE];

            while (fread(buf, sizeof(char), FILE_TRANSFER_BLOCK_SIZE, f) > 0) {
                // FIXME: need size so we don't print garbage
                printf("%s", buf);
            };
            fclose(f);
        }
        printf("\n\nA copy of this scoreboard has been saved at %s.\n", fname);
    } else if (status == RSB_EMPTY) {
        printf("The scoreboard is currently empty (no game was yet won by any "
               "player).\n");
    }
}

const void *UDPCommandDescriptorsIndexer(const void *arr, size_t i) {
    return &(((const UDPCommandDescriptor *)arr)[i]);
}

char **getUDPCommandAliases(const void *cmd) {
    const UDPCommandDescriptor *descr = (const UDPCommandDescriptor *)cmd;
    return descr->aliases;
}

size_t getUDPCommandAliasesCount(const void *cmd) {
    const UDPCommandDescriptor *descr = (const UDPCommandDescriptor *)cmd;
    return descr->aliasesCount;
}

const void *TCPCommandDescriptorsIndexer(const void *arr, size_t i) {
    return &(((const TCPCommandDescriptor *)arr)[i]);
}

char **getTCPCommandAliases(const void *cmd) {
    const TCPCommandDescriptor *descr = (const TCPCommandDescriptor *)cmd;
    return descr->aliases;
}

size_t getTCPCommandAliasesCount(const void *cmd) {
    const TCPCommandDescriptor *descr = (const TCPCommandDescriptor *)cmd;
    return descr->aliasesCount;
}