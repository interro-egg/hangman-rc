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

typedef void *(*CommandParser)(char *args);
typedef int (*CommandPreHook)(void *parsed, PlayerState *state);
typedef ssize_t (*CommandSerializer)(void *ptr, char *outBuffer);
typedef void *(*CommandDeserializer)(char *inBuffer);
typedef void (*CommandDestroyer)(void *ptr);
typedef void (*CommandCallback)(void *req, void *resp, PlayerState *state);

typedef struct {
    char **aliases;
    size_t aliasesCount;
    CommandParser argsParser;
    CommandPreHook preHook;
    CommandSerializer requestSerializer;
    CommandDestroyer requestDestroyer;
    CommandDeserializer responseDeserializer;
    CommandDestroyer responseDestroyer;
    CommandCallback callback;
} CommandDescriptor;

extern const CommandDescriptor COMMANDS[];
extern const size_t COMMANDS_COUNT;

int handleCommand(const CommandDescriptor *cmd, char *args, PlayerState *state);

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

#endif // COMMANDS_H