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
#define HANDLER_EDESERIALIZE -6

typedef void *(*CommandParser)(char *args);
typedef ssize_t (*CommandSerializer)(void *ptr, char *outBuffer);
typedef void *(*CommandDeserializer)(char *inBuffer);
typedef void (*CommandDestroyer)(void *ptr);
typedef int (*CommandCallback)(void *req, void *resp, PlayerState *state);

typedef struct {
    char **aliases;
    size_t aliasesCount;
    CommandParser argsParser;
    CommandSerializer requestSerializer;
    CommandDestroyer requestDestroyer;
    CommandDeserializer responseDeserializer;
    CommandDestroyer responseDestroyer;
    CommandCallback callback;
} CommandDescriptor;

extern const CommandDescriptor COMMANDS[];
extern const size_t COMMANDS_COUNT;

int handleCommand(const CommandDescriptor *cmd, char *args, PlayerState *state);

int startCallback(void *req, void *resp, PlayerState *state);
int playCallback(void *req, void *resp, PlayerState *state);
int guessCallback(void *req, void *resp, PlayerState *state);
int revealCallback(void *req, void *resp, PlayerState *state);
int scoreboardCallback(void *req, void *resp, PlayerState *state);
int hintCallback(void *req, void *resp, PlayerState *state);
int stateCallback(void *req, void *resp, PlayerState *state);
int quitCallback(void *req, void *resp, PlayerState *state);
int exitCallback(void *req, void *resp, PlayerState *state);

#endif // COMMANDS_H