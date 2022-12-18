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
#define HANDLER_ECOMMS_UDP -5
#define HANDLER_ECOMMS_TCP -6
#define HANDLER_ECOMMS_TIMEO -7
#define HANDLER_EDESERIALIZE -8

typedef void *(*CommandParser)(char *args);
typedef int (*CommandPreHook)(void *parsed, PlayerState *state);
typedef ssize_t (*CommandSerializer)(void *ptr, char *outBuffer);
typedef void (*CommandDestroyer)(void *ptr);

typedef void *(*UDPCommandDeserializer)(char *inBuffer);
typedef void (*UDPCommandCallback)(void *req, void *resp, PlayerState *state);

typedef void (*TCPCommandCallback)(void *req, int status, char *fname,
                                   PlayerState *state);

typedef struct {
    char **aliases;
    size_t aliasesCount;
    CommandParser argsParser;
    CommandPreHook preHook;
    CommandSerializer requestSerializer;
    CommandDestroyer requestDestroyer;
    UDPCommandDeserializer responseDeserializer;
    CommandDestroyer responseDestroyer;
    UDPCommandCallback callback;
} UDPCommandDescriptor;

typedef struct {
    char **aliases;
    size_t aliasesCount;
    CommandParser argsParser;
    CommandPreHook preHook;
    CommandSerializer requestSerializer;
    CommandDestroyer requestDestroyer;
    const char **statusEnumStrings;
    size_t maxStatusEnumLen;
    const bool *fileReceiveStatuses;
    TCPCommandCallback callback;
} TCPCommandDescriptor;

extern const UDPCommandDescriptor UDP_COMMANDS[];
extern const size_t UDP_COMMANDS_COUNT;

extern const TCPCommandDescriptor TCP_COMMANDS[];
extern const size_t TCP_COMMANDS_COUNT;

int handleUDPCommand(const UDPCommandDescriptor *cmd, char *args,
                     PlayerState *state);
int handleTCPCommand(const TCPCommandDescriptor *cmd, char *args,
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

typedef const void *(*CommandDescriptorsIndexer)(const void *arr, size_t i);
typedef char **(*CommandAliasesGetter)(const void *cmd);
typedef size_t (*CommandAliasesCountGetter)(const void *cmd);

const void *UDPCommandDescriptorsIndexer(const void *arr, size_t i);
char **getUDPCommandAliases(const void *cmd);
size_t getUDPCommandAliasesCount(const void *cmd);
const void *TCPCommandDescriptorsIndexer(const void *arr, size_t i);
char **getTCPCommandAliases(const void *cmd);
size_t getTCPCommandAliasesCount(const void *cmd);

#endif // COMMANDS_H