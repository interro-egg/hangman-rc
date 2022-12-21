#include "commands.h"
#include "../common/common.h"
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

char *hintCmdAliases[] = {"hint", "h"};
const TCPCommandDescriptor hintCmd = {hintCmdAliases,
                                      2,
                                      parseGHLArgs,
                                      hintPreHook,
                                      serializeGHLMessage,
                                      destroyGHLMessage,
                                      "RHL",
                                      RHLMessageStatusStrings,
                                      3,
                                      RHLMessageFileReceiveStatuses,
                                      hintCallback};

char *stateCmdAliases[] = {"state", "st"};
const TCPCommandDescriptor stateCmd = {stateCmdAliases,
                                       2,
                                       parseSTAArgs,
                                       statePreHook,
                                       serializeSTAMessage,
                                       destroySTAMessage,
                                       "RST",
                                       RSTMessageStatusStrings,
                                       3,
                                       RSTMessageFileReceiveStatuses,
                                       stateCallback};

const TCPCommandDescriptor TCP_COMMANDS[] = {scoreboardCmd, hintCmd, stateCmd};
const size_t TCP_COMMANDS_COUNT = 3;

int handleUDPCommand(const UDPCommandDescriptor *cmd, char *args,
                     PlayerState *state) {
    errno = 0;
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
        int r = (errno == EAGAIN || errno == EWOULDBLOCK || errno == ETIMEDOUT)
                    ? HANDLER_ECOMMS_TIMEO
                    : HANDLER_ECOMMS_UDP;
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
    errno = 0;
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
        switch (errno) {
        case TCP_SND_ESNDTIMEO:
            return HANDLER_ECOMMS_TIMEO;
        case TCP_SND_ESOCKET:
        case TCP_SND_EWRITE:
        case TCP_SND_ECONNECT:
        default:
            return HANDLER_ECOMMS_TCP;
        }
    }

    bool foundEOL = false;
    ssize_t r = readWordTCP(fd, state->in_buffer, MESSAGE_COMMAND_SIZE, false,
                            &foundEOL);
    if (r <= 0 || foundEOL) {
        errno = (int)r;
        cmd->requestDestroyer(parsed);
        close(fd);
        state->tcp_socket = -1;
        switch (errno) {
        case TCP_RCV_EINV:
            return HANDLER_EDESERIALIZE;
        case TCP_RCV_EREAD:
        default:
            return HANDLER_ECOMMS_TCP;
        }
    }
    if (strcmp(state->in_buffer, cmd->expectedResponse) != 0) {
        cmd->requestDestroyer(parsed);
        close(fd);
        state->tcp_socket = -1;
        return HANDLER_EDESERIALIZE;
    }

    r = readWordTCP(fd, state->in_buffer, cmd->maxStatusEnumLen, false,
                    &foundEOL);
    if (r <= 0) {
        errno = (int)r;
        cmd->requestDestroyer(parsed);
        close(fd);
        state->tcp_socket = -1;
        switch (errno) {
        case TCP_RCV_EINV:
            return HANDLER_EDESERIALIZE;
        case TCP_RCV_EREAD:
        default:
            return HANDLER_ECOMMS_TCP;
        }
    }

    int status = parseEnum(cmd->statusEnumStrings, state->in_buffer);
    if (status == -1) {
        cmd->requestDestroyer(parsed);
        close(fd);
        state->tcp_socket = -1;
        return HANDLER_EDESERIALIZE;
    }

    ReceivedFile *file = NULL;
    if (cmd->fileReceiveStatuses[status]) {
        errno = 0;
        if ((file = readFileTCP(fd)) == NULL) {
            cmd->requestDestroyer(parsed);
            close(fd);
            state->tcp_socket = -1;
            switch (errno) {
            case TCP_RCV_ENOMEM:
                return HANDLER_ENOMEM;
            case TCP_RCV_EINV:
                return HANDLER_EDESERIALIZE;
            case TCP_RCV_ETIMEO:
                return HANDLER_ECOMMS_TIMEO;
            default:
                return HANDLER_ECOMMS_TCP;
            }
        }

        errno = 0;
        if (checkNewline(fd) != 0) {
            bool timedOut = errno == EINPROGRESS;
            unlink(file->fname);
            // no need to check for error as we're already returning error;
            // this is a best-effort attempt
            destroyReceivedFile(file);
            cmd->requestDestroyer(parsed);
            close(fd);
            state->tcp_socket = -1;
            return timedOut ? HANDLER_ECOMMS_TIMEO : HANDLER_EDESERIALIZE;
        }
    } else if (!foundEOL) {
        cmd->requestDestroyer(parsed);
        close(fd);
        state->tcp_socket = -1;
        return HANDLER_EDESERIALIZE;
    }

    if (cmd->callback != NULL) {
        cmd->callback(parsed, status, file, state);
    }

    cmd->requestDestroyer(parsed);
    close(fd);
    state->tcp_socket = -1;
    destroyReceivedFile(file);
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
    switch (rsg->status) {
    case RSG_OK:
        if (startGame(state, rsg->n_letters, rsg->max_errors, sng->PLID) ==
            -1) {
            printf("Failed to start game. Please try again.");
        }
        printf("New game started. Guess %u letter word: ", rsg->n_letters);
        displayWord(state->word);
        printf("\nYou have %u wrong guesses left.\n", state->remaining_errors);
        return;
    case RSG_NOK:
    case RSG_ERR:
    default:
        printf("A game could not be started. You might have specified an "
               "invalid PLID; please try again.\n");
        return;
    }
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
    switch (rlg->status) {
    case RLG_OK:
        for (size_t i = 0; i < rlg->n; i++) {
            state->word[rlg->pos[i] - 1] = (char)toupper(plg->letter);
        }
        printf("Correct!\n");
        break;
    case RLG_WIN:
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
    case RLG_DUP:
        printf("You have already guessed this letter. Please try again.\n");
        state->trial--;
        break;
    case RLG_NOK:
        printf("Wrong letter. Please try again.\n");
        state->remaining_errors--;
        break;
    case RLG_OVR:
        printf("Wrong letter. You have exceeded the maximum number of errors. "
               "You lost!\n");
        endGame(state);
        return;
    case RLG_INV:
        printf("An error occurred. Please try again.\n");
        state->trial = rlg->trial; // Try to sync with server
        return;
    case RLG_ERR:
    default:
        printf("An error occurred. Please try again. You can run 'state' to "
               "check the current game state.\n");
        state->trial--;
        return;
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
    size_t len = strlen(state->word);
    if (strlen(pwg->word) != len) {
        printf(
            "The specified word is invalid: the target word has %ld letters.\n",
            len);
        return -1;
    }
    strncpy(pwg->PLID, state->PLID, 6 + 1);
    pwg->trial = ++state->trial;
    return 0;
}

void guessCallback(void *req, void *resp, PlayerState *state) {
    PWGMessage *pwg = (PWGMessage *)req;
    RWGMessage *rwg = (RWGMessage *)resp;
    switch (rwg->status) {
    case RWG_WIN:
        strncpy(state->word, pwg->word, strlen(state->word) + 1);
        for (size_t i = 0; i < strlen(state->word); i++) {
            state->word[i] = (char)toupper(state->word[i]);
        }
        displayWord(state->word);
        printf("\nYou won! Congratulations!\n");
        endGame(state);
        return;
    case RWG_DUP:
        printf("You have already guessed this word. Please try again.\n");
        state->trial--;
        break;
    case RWG_NOK:
        printf("Wrong word. Please try again.\n");
        state->remaining_errors--;
        break;
    case RWG_OVR:
        printf("Wrong word. You have exceeded the maximum number of errors. "
               "You lost!\n");
        endGame(state);
        return;
    case RWG_INV:
        printf("An error occurred. Please try again.\n");
        state->trial = rwg->trials; // Try to sync with server
        break;
    case RWG_ERR:
    default:
        printf("An error occurred. Please try again. You can run 'state' to "
               "check the current game state.\n");
        state->trial--;
        return;
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
    switch (rqt->status) {
    case RQT_OK:
        printf("You have quit the game.\n");
        endGame(state);
        return;
    case RQT_NOK:
        printf("You do not have a game to quit. Please start a new one.\n");
        return;
    case RQT_ERR:
    default:
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

void exitCallback(UNUSED void *req, UNUSED void *resp, PlayerState *state) {
    endGame(state);
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
    switch (rrv->type) {
    case RRV_WORD:
        printf("The word is: ");
        for (size_t i = 0; i < strlen(rrv->data.word); i++) {
            rrv->data.word[i] = (char)toupper(rrv->data.word[i]);
        }
        displayWord(rrv->data.word);
        putchar('\n');
        return;
    case RRV_STATUS:
    default:
        switch (rrv->data.status) {
        case RRV_OK:
            printf("This feature is not currently enabled.\n");
            return;
        case RRV_ERR:
        default:
            printf("An error occurred. Please try again.\n");
        }
    }
}

void scoreboardCallback(UNUSED void *req, int status, ReceivedFile *file,
                        UNUSED PlayerState *state) {
    switch (status) {
    case RSB_OK:
        putchar('\n');
        displayFile(file);
        printf("\nA copy of this scoreboard has been saved at %s (%ld B).\n",
               file->fname, file->fsize);
        break;
    case RSB_EMPTY:
    default:
        printf("The scoreboard is currently empty (no game was yet won by any "
               "player).\n");
    }
}

int hintPreHook(void *req, PlayerState *state) {
    if (!state->in_game) {
        printf("You are not in a game. Please start a new one.\n");
        return -1;
    }
    GHLMessage *ghl = (GHLMessage *)req;
    strncpy(ghl->PLID, state->PLID, 6 + 1);
    return 0;
}

void hintCallback(UNUSED void *req, int status, ReceivedFile *file,
                  UNUSED PlayerState *state) {
    switch (status) {
    case RHL_OK:
        printf("A hint image illustrating the class to which the word belongs "
               "has been saved at %s (%ld B).\n",
               file->fname, file->fsize);
        break;
    case RHL_NOK:
    default:
        printf("A problem has occurred or there is no hint image available for "
               "this word.\n");
    }
}

int statePreHook(void *req, PlayerState *state) {
    if (state->PLID == NULL) {
        printf("You are not in a game, nor have you ever played a game. Please "
               "start a new one.\n");
        return -1;
    }
    STAMessage *sta = (STAMessage *)req;
    strncpy(sta->PLID, state->PLID, 6 + 1);
    return 0;
}

void stateCallback(UNUSED void *req, int status, ReceivedFile *file,
                   PlayerState *state) {
    switch (status) {
    case RST_ACT:
        printf("Here is a summary of the ongoing game:\n\n");
        displayFile(file);
        break;
    case RST_FIN:
        printf("There is no ongoing game, so here is a summary of the most "
               "recently finished game for the specified PLID:\n");
        displayFile(file);
        endGame(state);
        break;
    case RST_NOK:
    default:
        printf("A problem has occurred or there is no hint image available for "
               "this word.\n");
        return;
    }
    printf("\nA copy of the above game state been saved at %s (%ld B).\n",
           file->fname, file->fsize);
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

void displayWord(char *word) {
    if (word == NULL) {
        printf("No word found.\n");
        return;
    }
    for (size_t i = 0; word[i] != '\0'; i++) {
        printf("%s%c", i != 0 ? " " : "", word[i]);
    }
}

void displayFile(ReceivedFile *file) {
    FILE *f = fopen(file->fname, "r");
    if (f == NULL) {
        printf("Failed to display file.\n");
    } else {
        char buf[FILE_TRANSFER_BLOCK_SIZE];

        size_t r, remainingRead = file->fsize;
        while ((r = fread(buf, sizeof(char),
                          MIN(FILE_TRANSFER_BLOCK_SIZE, remainingRead), f)) >
               0) {
            remainingRead -= r;

            size_t remainingWrite = r;
            ssize_t w;
            while ((w = write(1, buf, remainingWrite)) > 0) {
                remainingWrite -= (size_t)w;
            }
        };
        fclose(f);
    }
}