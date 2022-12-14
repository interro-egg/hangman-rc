#include "commands.h"
#include "../common/messages.h"
#include "parsers.h"
#include "utils.h"
#include <errno.h>
#include <stdio.h>

int startHandler(char *args, UNUSED PlayerState *state) {
    // TODO: check return values
    SNGMessage *sng = parseSNGArgs(args);
    if (sng == NULL) {
        return (errno == ENOMEM) ? HANDLER_ENOMEM : HANDLER_EPARSE;
    }
    ssize_t req = serializeSNGMessage(sng, state->out_buffer);
    if (req < 0) {
        return HANDLER_ESERIALIZE;
    }
    char *resp = sendUDPMessage(state, state->out_buffer);
    if (resp == NULL) {
        return HANDLER_ECOMMS;
    }
    RSGMessage *rsg = deserializeRSGMessage(resp);
    if (rsg == NULL) {
        return HANDLER_EDESERIALIZE;
    }

    switch (rsg->status) {
    case RSG_OK:
        state->PLID = sng->PLID;
        // state->n_letters = rsg->n_letters;
        // state->max_errors = rsg->max_errors;
        break;
    case RSG_NOK:
        // TODO
        break;
    default:
        return HANDLER_EUNKNOWN;
    }

    return HANDLER_SUCCESS;
}

int playHandler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

int guessHandler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

int revealHandler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

int scoreboardHandler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

int hintHandler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

int stateHandler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

int quitHandler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

int exitHandler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}
