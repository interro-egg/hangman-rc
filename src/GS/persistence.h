#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "server_state.h"
#include <stddef.h>

typedef struct {
    char *word;
    char *hintFile;
} WordListEntry;

typedef struct {
    size_t numEntries;
    WordListEntry *entries;
} WordList;

enum GameOutcome {
    OUTCOME_ONGOING = '-',
    OUTCOME_WIN = 'W',
    OUTCOME_FAIL = 'F',
    OUTCOME_QUIT = 'Q'
};

enum GameTrialType {
    TRIAL_TYPE_LETTER = 'L',
    TRIAL_TYPE_WORD = 'W',
};

typedef struct {
    enum GameTrialType type;
    union {
        char letter;
        char *word;
    } guess;
    bool correct;
} GameTrial;

typedef struct {
    char *__curFilePath;
    char *PLID;
    enum GameOutcome outcome;
    char *finishStamp;
    WordListEntry *wordListEntry;
    size_t numTrials;
    GameTrial *trials;
} Game;

void destroyWordListEntry(WordListEntry *entry);

WordList *getWordList(ServerState *state);
void destroyWordList(WordList *list);
WordListEntry *chooseWordListEntry(WordList *list, ServerState *state);

void destroyGameTrial(GameTrial *trial);

Game *newGame(char *PLID, ServerState *state);
void destroyGame(Game *game);
char *computeGameFilePath(Game *game);
int saveGame(Game *game, ServerState *state);
Game *loadGame(char *PLID, ServerState *state);
int registerGameTrial(Game *game, GameTrial *trial, ServerState *state);

#endif // PERSISTENCE_H