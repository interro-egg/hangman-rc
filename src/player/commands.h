#ifndef COMMANDS_H
#define COMMANDS_H

#include "../common/common.h"
#include "player_state.h"
#include <stdbool.h>
#include <sys/types.h>

#define HANDLER_SUCCESS 0
#define HANDLER_EUNKNOWN -1
#define HANDLER_ENOMEM -2
#define HANDLER_EPARSE -3
#define HANDLER_ESERIALIZE -4
#define HANDLER_ECOMMS -5
#define HANDLER_ECOMMS_TIMEO -6
#define HANDLER_EDESERIALIZE -7

typedef void *(*UDPCommandParser)(char *args);
typedef int (*UDPCommandPreHook)(void *parsed, PlayerState *state);
typedef ssize_t (*UDPCommandSerializer)(void *ptr, char *outBuffer);
typedef void *(*UDPCommandDeserializer)(char *inBuffer);
typedef void (*UDPCommandDestroyer)(void *ptr);
typedef void (*UDPCommandCallback)(void *req, void *resp, PlayerState *state);

typedef struct {
    char **aliases;
    size_t aliasesCount;
    UDPCommandParser argsParser;
    UDPCommandPreHook preHook;
    UDPCommandSerializer requestSerializer;
    UDPCommandDestroyer requestDestroyer;
    UDPCommandDeserializer responseDeserializer;
    UDPCommandDestroyer responseDestroyer;
    UDPCommandCallback callback;
} UDPCommandDescriptor;

extern const UDPCommandDescriptor UDP_COMMANDS[];
extern const size_t UDP_COMMANDS_COUNT;

int handleUDPCommand(const UDPCommandDescriptor *cmd, char *args,
                     PlayerState *state);

int startPreHook(void *parsed, PlayerState *state);
void startCallback(void *req, void *resp, PlayerState *state);
int playPreHook(void *parsed, PlayerState *state);
void playCallback(void *req, void *resp, PlayerState *state);
int guessPreHook(void *parsed, PlayerState *state);
void guessCallback(void *req, void *resp, PlayerState *state);
void revealCallback(void *req, void *resp, PlayerState *state);
void scoreboardCallback(void *req, void *resp, PlayerState *state);
void hintCallback(void *req, void *resp, PlayerState *state);
void stateCallback(void *req, void *resp, PlayerState *state);
int quitPreHook(void *parsed, PlayerState *state);
void quitCallback(void *req, void *resp, PlayerState *state);
int exitPreHook(void *parsed, PlayerState *state);
void exitCallback(void *req, void *resp, PlayerState *state);
int revealPreHook(void *req, PlayerState *state);
void revealCallback(void *req, void *resp, PlayerState *state);

#endif // COMMANDS_H