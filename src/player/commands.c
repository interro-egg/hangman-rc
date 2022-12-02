#include "commands.h"
#include <stdio.h>

bool start_handler(char *args, UNUSED PlayerState *state) {
    // SNGMessage *sng = parseSNGArgs(args);
    // char *req = serializeSNGMessage(sng);
    // char *resp = sendUDPMessage(state, req, "RSG");
    // RSG *rsg = deserializeRSGMessage(resp);
    // state->PLID = args->PLID;
    // printf(rsg->cenas);
    printf("Start! Args = |%s|\n", args);
    return true;
}

bool play_handler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

bool guess_handler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

bool reveal_handler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

bool scoreboard_handler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

bool hint_handler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

bool state_handler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

bool quit_handler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}

bool exit_handler(char *args, UNUSED PlayerState *state) {
    printf("Play! Args = |%s|\n", args);
    return true;
}
