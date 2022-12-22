#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "server_state.h"
#include <dirent.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define UMASK 0022

#define HINTS_DIR "hints"
#define GAMES_DIR "GAMES"
#define SCORES_DIR "SCORES"
#define SCOREBOARD_FILE_NAME "SCOREBOARD.txt"
#define SCOREBOARD_MAX_ITEMS 10

#define MAX_FILE_PATH_SIZE 37 + 1
#define MAX_TIMESTAMP_SIZE (sizeof("YYYYMMDD_HHMMSS"))

typedef struct {
    char *name;
    size_t size;
    char *data;
} ResponseFile;

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
    bool correct;
} GameTrial;

typedef struct {
    char *PLID;
    enum GameOutcome outcome;
    WordListEntry *wordListEntry;
    char *maskedWord;
    unsigned int numTrials;
    GameTrial **trials;
    unsigned int numSucc; // number of successful trials
    unsigned int maxErrors;
    unsigned int remainingLetters;
} Game;

typedef struct {
    unsigned int score;
    char *PLID;
    char *finishStamp;
    char *word;
    unsigned int numSucc;
    unsigned int numTrials;
} Score;

const char *translateGameOutcome(enum GameOutcome outcome);

int initPersistence();

WordList *parseWordListFile(char *wordFile);
void destroyWordList(WordList *list);

WordListEntry *createWordListEntry(char *word, char *hintFile);
void destroyWordListEntry(WordListEntry *entry);

WordListEntry *chooseWordListEntry(ServerState *state);
WordListEntry *chooseRandomWordListEntry(WordList *list);
WordListEntry *chooseSequentialWordListEntry(WordList *list, size_t *seqPtr);

Game *newGame(char *PLID, ServerState *state);
void destroyGame(Game *game);

int saveGame(Game *game);
Game *loadGame(char *PLID, bool ongoingOnly);
char *computeGameFilePath(char *PLID, bool ongoing);
FILE *findGameFileForPlayer(char *PLID, bool ongoingOnly);
int endGame(Game *game, enum GameOutcome outcome);

int registerGameTrial(Game *game, GameTrial *trial);
void destroyGameTrial(GameTrial *trial);

Score *newScore(Game *game);
void destroyScore(Score *score);
unsigned int calculateScore(Game *game);
int registerScore(Score *score);
Score *loadScore(char *filePath);

int generateScoreboard();
ResponseFile *getScoreboard();
ResponseFile *getGameState(Game *game);
ResponseFile *getResponseFile(FILE *file, char *responseFileName);
void destroyResponseFile(ResponseFile *resp);
ResponseFile *getFSResponseFile(char *dirPath, char *fileName,
                                char *responseFileName);

int isNotScoreboardFile(const struct dirent *entry);
int ensureDirExists(const char *path);
char *formattedTimeStamp();

#endif // PERSISTENCE_H