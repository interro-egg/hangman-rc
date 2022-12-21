#include "persistence.h"
#include "../common/common.h"
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

int initPersistence() {
    if (ensureDirExists("GAMES") != 0 || ensureDirExists("SCORES") != 0) {
        return -1;
    }
    umask(UMASK);
    return 0;
}

Game *newGame(char *PLID, ServerState *state) {
    Game *game = malloc(sizeof(Game));
    if (game == NULL) {
        return NULL;
    }
    game->PLID = PLID;
    game->outcome = OUTCOME_ONGOING;
    if ((game->wordListEntry = chooseWordListEntry(state)) == NULL) {
        return NULL;
    }
    game->numTrials = 0;
    game->trials = NULL;
    game->numSucc = 0;
    game->maxErrors = calculateMaxErrors(game->wordListEntry->word);
    game->remainingLetters = (unsigned int)strlen(game->wordListEntry->word);
    return game;
}

void destroyGame(Game *game) {
    if (game == NULL) {
        return;
    }
    free(game->PLID);
    destroyWordListEntry(game->wordListEntry);
    for (size_t i = 0; i < game->numTrials; i++) {
        destroyGameTrial(game->trials[i]);
    }
    free(game);
}

void destroyWordListEntry(WordListEntry *entry) {
    if (entry == NULL) {
        return;
    }
    free(entry->word);
    free(entry->hintFile);
    free(entry);
}

void destroyWordList(WordList *list) {
    if (list == NULL) {
        return;
    }
    for (size_t i = 0; i < list->numEntries; i++) {
        destroyWordListEntry(list->entries[i]);
    }
    free(list);
}

void destroyGameTrial(GameTrial *trial) {
    if (trial == NULL) {
        return;
    }
    if (trial->type == TRIAL_TYPE_WORD) {
        free(trial->guess.word);
    }
    free(trial);
}

WordListEntry *chooseWordListEntry(ServerState *state) {
    if (state->sequential_word_selection) {
        return chooseSequentialWordListEntry(state->word_list,
                                             &state->word_list_seq_ptr);
    } else {
        return chooseRandomWordListEntry(state->word_list);
    }
}

WordListEntry *chooseRandomWordListEntry(WordList *list) {
    if (list == NULL || list->numEntries == 0) {
        return NULL;
    }
    size_t index = (size_t)rand() % list->numEntries;
    return list->entries[index];
}

WordListEntry *chooseSequentialWordListEntry(WordList *list, size_t *seqPtr) {
    if (list == NULL || list->numEntries == 0) {
        return NULL;
    }
    *seqPtr = (*seqPtr) % list->numEntries;
    return list->entries[(*seqPtr)++]; // 🤯
}

WordListEntry *createWordListEntry(char *word, char *hintFile) {
    WordListEntry *entry = malloc(sizeof(WordListEntry));
    if (entry == NULL || word == NULL || hintFile == NULL) {
        return NULL;
    }
    entry->word = strdup(word);
    entry->hintFile = strdup(hintFile);
    if (entry->word == NULL || entry->hintFile == NULL) {
        free(entry->word);
        free(entry->hintFile);
        return NULL;
    }
    return entry;
}

WordList *parseWordListFile(char *wordFile) {
    FILE *file = fopen(wordFile, "r");
    if (file == NULL) {
        return NULL;
    }
    WordList *list = malloc(sizeof(WordList));
    if (list == NULL) {
        return NULL;
    }
    list->numEntries = 0;
    list->entries = NULL;

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, file)) != -1) {
        char *word = strtok(line, " \n");
        char *hintFile = strtok(NULL, " \n");
        if (word == NULL || hintFile == NULL) {
            return NULL;
        }
        WordListEntry *entry = createWordListEntry(word, hintFile);
        if (entry == NULL) {
            return NULL;
        }
        list->numEntries++;
        list->entries = reallocarray(list->entries, list->numEntries,
                                     sizeof(WordListEntry));
        if (list->entries == NULL) {
            return NULL;
        }
        list->entries[list->numEntries - 1] = entry;
    }
    free(line);
    fclose(file);
    return list;
}

int registerGameTrial(Game *game, GameTrial *trial) {
    if (game == NULL || trial == NULL) {
        return -1;
    }
    game->numTrials++;
    game->trials =
        reallocarray(game->trials, game->numTrials, sizeof(GameTrial *));
    if (game->trials == NULL) {
        return -1;
    }
    game->trials[game->numTrials - 1] = trial;
    return 0;
}

int saveGame(Game *game) {
    if (game == NULL) {
        return -1;
    }
    char *filePath =
        computeGameFilePath(game->PLID, game->outcome == OUTCOME_ONGOING);
    if (filePath == NULL) {
        return -1;
    }
    FILE *file = fopen(filePath, "w");
    if (file == NULL || flock(fileno(file), LOCK_EX) == -1) {
        return -1;
    }
    if (fprintf(file, "%s %s\n%c %u %d %u\n", game->wordListEntry->word,
                game->wordListEntry->hintFile, game->outcome, game->numSucc,
                game->maxErrors, game->remainingLetters) <= 0) {
        fclose(file);
        return -1;
    }
    for (size_t i = 0; i < game->numTrials; i++) {
        if (game->trials[i]->type == TRIAL_TYPE_LETTER) {
            if (fprintf(file, "%c %c\n", game->trials[i]->type,
                        game->trials[i]->guess.letter) <= 0) {
                fclose(file);
                return -1;
            }
        } else if (game->trials[i]->type == TRIAL_TYPE_WORD) {
            if (fprintf(file, "%c %s\n", game->trials[i]->type,
                        game->trials[i]->guess.word) <= 0) {
                fclose(file);
                return -1;
            }
        }
    }

    fclose(file);
    return 0;
}

Game *loadGame(char *PLID, bool ongoingOnly) {
    if (PLID == NULL) {
        return NULL;
    }
    FILE *file = findGameFileForPlayer(PLID, ongoingOnly);
    if (file == NULL || flock(fileno(file), LOCK_EX) == -1) {
        return NULL;
    }

    Game *game = malloc(sizeof(Game));
    if (game == NULL) {
        fclose(file);
        return NULL;
    }

    char *word = NULL;
    char *hintFile = NULL;
    char outcome;
    if (fscanf(file, "%ms %ms\n%c %u %d %u\n", &word, &hintFile, &outcome,
               &game->numSucc, &game->maxErrors,
               &game->remainingLetters) != 6) {
        destroyGame(game);
        fclose(file);
        return NULL;
    }
    game->outcome = (enum GameOutcome)outcome;
    game->wordListEntry = createWordListEntry(word, hintFile);
    if (game->wordListEntry == NULL) {
        destroyGame(game);
        fclose(file);
        return NULL;
    }
    game->numTrials = 0;
    game->trials = NULL;
    game->PLID = strdup(PLID);

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char type;
    char *guess = malloc((MAX_WORD_SIZE + 1) * sizeof(char));
    while ((read = getline(&line, &len, file)) != -1) {
        if (sscanf(line, "%c %s\n", &type, guess) != 2) {
            destroyGame(game);
            fclose(file);
            return NULL;
        }
        GameTrial *trial = malloc(sizeof(GameTrial));
        trial->type = type;
        if (type == TRIAL_TYPE_LETTER) {
            trial->guess.letter = guess[0];
        } else if (type == TRIAL_TYPE_WORD) {
            trial->guess.word = guess;
        }
        if (registerGameTrial(game, trial) == -1) {
            destroyGame(game);
            fclose(file);
            return NULL;
        }
    }

    free(line);
    fclose(file);
    return game;
}

unsigned int calculateScore(Game *game) {
    if (game == NULL || game->numSucc == 0 || game->numTrials == 0) {
        return 0;
    }
    // round(game->numSucc / game->numTrials)
    unsigned int rounded =
        ((100 * game->numSucc) + (game->numTrials / 2)) / game->numTrials;

    return MIN(100, rounded);
}

int endGame(Game *game, enum GameOutcome outcome) {
    if (game == NULL || game->outcome != OUTCOME_ONGOING) {
        return -1;
    }
    game->outcome = outcome;

    if (outcome == OUTCOME_WIN) {
        Score *score = newScore(game);
        if (score == NULL) {
            return -1;
        }
        if (registerScore(score) == -1) {
            destroyScore(score);
            return -1;
        }
        destroyScore(score);
    }

    // will save at new location
    if (saveGame(game) == -1) {
        return -1;
    }
    return unlink(computeGameFilePath(game->PLID, true));
}

// Either ongoing or last game
FILE *findGameFileForPlayer(char *PLID, bool ongoingOnly) {
    if (PLID == NULL) {
        return NULL;
    }
    char *filePath = computeGameFilePath(PLID, true);
    if (filePath == NULL) {
        return NULL;
    }
    FILE *file = fopen(filePath, "r");
    free(filePath);
    if (file == NULL) {
        if (errno != ENOENT || ongoingOnly) {
            return NULL;
        }
        filePath = computeGameFilePath(PLID, false);
        if (filePath == NULL) {
            return NULL;
        }
        file = fopen(filePath, "r");
        if (file == NULL) {
            return NULL;
        }
    }

    return file;
}

char *computeGameFilePath(char *PLID, bool ongoing) {
    if (PLID == NULL) {
        return NULL;
    }
    char *filePath = malloc(MAX_FILE_PATH_SIZE * sizeof(char));
    if (filePath == NULL) {
        return NULL;
    }
    snprintf(filePath, MAX_FILE_PATH_SIZE,
             ongoing ? "%s/%s.txt" : "%s/%s_last.txt", GAMES_DIR, PLID);
    return filePath;
}

Score *newScore(Game *game) {
    if (game == NULL || game->outcome == OUTCOME_ONGOING) {
        return NULL;
    }
    Score *score = malloc(sizeof(Score));
    if (score == NULL) {
        return NULL;
    }

    score->score = calculateScore(game);
    score->PLID = strdup(game->PLID);
    score->finishStamp = formattedTimeStamp();
    score->word = strdup(game->wordListEntry->word);
    score->numSucc = game->numSucc;
    score->numTrials = game->numTrials;

    if (score->PLID == NULL || score->finishStamp == NULL ||
        score->word == NULL) {
        destroyScore(score);
        return NULL;
    }

    return score;
}

void destroyScore(Score *score) {
    if (score == NULL) {
        return;
    }
    free(score->PLID);
    free(score->finishStamp);
    free(score->word);
    free(score);
}

int registerScore(Score *score) {
    if (score == NULL) {
        return -1;
    }
    char *filePath = malloc(MAX_FILE_PATH_SIZE * sizeof(char));
    if (filePath == NULL) {
        return -1;
    }
    if (snprintf(filePath, MAX_FILE_PATH_SIZE, "%s/%03u_%s_%s.txt", SCORES_DIR,
                 score->score, score->PLID, score->finishStamp) <= 0) {
        free(filePath);
        return -1;
    }

    FILE *file = fopen(filePath, "w");
    free(filePath);

    if (file == NULL || flock(fileno(file), LOCK_EX) == -1) {
        return -1;
    }
    if (fprintf(file, "%s %u %u", score->word, score->numSucc,
                score->numTrials) == -1) {
        fclose(file);
        return -1;
    }

    fclose(file);
    return generateScoreboard();
}

Score *loadScore(char *filePath) {
    if (filePath == NULL) {
        return NULL;
    }
    FILE *file = fopen(filePath, "r");
    if (file == NULL || flock(fileno(file), LOCK_SH) == -1) {
        return NULL;
    }

    Score *score = malloc(sizeof(Score));
    char *word = malloc((MAX_WORD_SIZE + 1) * sizeof(char));
    if (score == NULL || word == NULL) {
        fclose(file);
        return NULL;
    }
    score->word = word;

    char *fileName = strrchr(filePath, '/') + 1;
    if (fileName == NULL) {
        free(score);
        fclose(file);
        return NULL;
    }

    char *scoreStr = strtok(fileName, "_");
    score->PLID = strtok(NULL, "_");
    score->finishStamp = strtok(NULL, ".");
    if (scoreStr == NULL || score->PLID == NULL || score->finishStamp == NULL ||
        sscanf(scoreStr, "%u", &score->score) != 1 ||
        fscanf(file, "%s %u %u", score->word, &score->numSucc,
               &score->numTrials) != 3) {
        free(score);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return score;
}

int generateScoreboard() {
    FILE *file = fopen(SCOREBOARD_FILE, "w");
    if (file == NULL || flock(fileno(file), LOCK_EX) == -1) {
        return -1;
    }

    struct dirent **namelist;
    int n = scandir(SCORES_DIR, &namelist, isNotScoreboardFile, alphasort);
    if (n <= 0) {
        fclose(file);
        unlink(SCOREBOARD_FILE);
        return n;
    }

    fprintf(file, "------------------------------- TOP 10 SCORES "
                  "-------------------------------\n\n");
    fprintf(file,
            "     SCORE  PLAYER  WORD                            GOOD TRIALS  "
            "TOTAL TRIALS\n\n");

    char filePath[MAX_FILE_PATH_SIZE];

    for (int i = n - 1, c = 1; i >= MAX(0, n - SCOREBOARD_MAX_ITEMS);
         i--, c++) {
        if (snprintf(filePath, MAX_FILE_PATH_SIZE, "%s/%s", SCORES_DIR,
                     namelist[i]->d_name) <= 0) {
            continue;
        }
        free(namelist[i]);
        Score *score = loadScore(filePath);
        if (score == NULL) {
            continue;
        }
        fprintf(file, "%2d - %5u  %-6s  %-30s  %11u  %12u\n", c, score->score,
                score->PLID, score->word, score->numSucc, score->numTrials);
        free(score);
    }
    free(namelist);

    fclose(file);
    return 0;
}

int isNotScoreboardFile(const struct dirent *entry) {
    return entry->d_name[0] != '.' &&
           strcmp(entry->d_name, SCOREBOARD_FILE_NAME) != 0;
}

int ensureDirExists(const char *path) {
    // check if directory exists
    if (access(path, R_OK || W_OK) == -1) {
        // if not, create it
        if (mkdir(path, 0755) == -1) {
            return -1;
        }
    }
    return 0;
}

char *formattedTimeStamp() {
    char *stamp = malloc(MAX_TIMESTAMP_SIZE * sizeof(char));
    if (stamp == NULL) {
        return NULL;
    }
    time_t now = time(NULL);
    if (strftime(stamp, MAX_TIMESTAMP_SIZE, "%Y%m%d_%H%M%S", gmtime(&now)) <=
        0) {
        free(stamp);
        return NULL;
    }
    return stamp;
}