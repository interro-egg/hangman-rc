#include "commands.h"
#include <stdio.h>

bool startHandler(char *args, UNUSED PlayerState *state) {
    // SNGMessage *sng = parseSNGArgs(args);
    // char *req = serializeSNGMessage(sng);
    // char *resp = sendUDPMessage(state, req, "RSG");
    // RSG *rsg = deserializeRSGMessage(resp);
    // state->PLID = args->PLID;
    // printf(rsg->cenas);
    printf("Start! Args = |%s|\n", args);
    return true;
}

bool playHandler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

bool guessHandler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

bool revealHandler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

bool scoreboardHandler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

bool hintHandler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

bool stateHandler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

bool quitHandler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

bool exitHandler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}
