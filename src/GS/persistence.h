#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "server_state.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define UMASK 0022

#define GAMES_DIR "GAMES"
#define SCORES_DIR "SCORES"
#define MAX_FILE_PATH_SIZE 37 + 1
// FIXME: this should be in common.h
#define MAX_GUESS_SIZE 30 + 1

typedef struct {
    char *word;
    char *hintFile;
} WordListEntry;

typedef struct WordList {
    size_t numEntries;
    WordListEntry **entries;
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
    union Guess {
        char letter;
        char *word;
    } guess;
} GameTrial;

typedef struct {
    char *PLID;
    enum GameOutcome outcome;
    WordListEntry *wordListEntry;
    unsigned int numTrials;
    GameTrial **trials;
    unsigned int numSucc; // number of successful trials
    unsigned int maxErrors;
    unsigned int remainingLetters;
} Game;

typedef struct {
    unsigned long score;
    char *PLID;
    char *finishStamp;
    char *word;
    unsigned int numSucc;
    unsigned int numTrials;
} Score;

int initPersistence();

void destroyWordListEntry(WordListEntry *entry);

WordListEntry *createWordListEntry(char *word, char *hintFile);
WordList *parseWordListFile(char *wordFile);

// TODO: delete this comment, reorder
void destroyWordList(WordList *list);
WordListEntry *chooseWordListEntry(ServerState *state);
WordListEntry *chooseRandomWordListEntry(WordList *list);
WordListEntry *chooseSequentialWordListEntry(WordList *list, size_t *seqPtr);

void destroyGameTrial(GameTrial *trial);

Game *newGame(char *PLID, ServerState *state);
void destroyGame(Game *game);

int endGame(Game *game, enum GameOutcome outcome);

char *computeGameFilePath(char *PLID, bool ongoing);
int saveGame(Game *game);
Game *loadGame(char *PLID, bool ongoingOnly);
int registerGameTrial(Game *game, GameTrial *trial);

FILE *findGameFileForPlayer(char *PLID, bool ongoingOnly);
int ensureDirExists(const char *path);

#endif // PERSISTENCE_H