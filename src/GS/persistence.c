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

const char *gameOutcomeStrings[] = {"ONGOING", "WIN", "FAIL", "QUIT"};

const char *translateGameOutcome(enum GameOutcome outcome) {
    switch (outcome) {
    case OUTCOME_ONGOING:
        return "ONGOING";
    case OUTCOME_WIN:
        return "WIN";
    case OUTCOME_FAIL:
        return "FAIL";
    case OUTCOME_QUIT:
        return "QUIT";
    default:
        return "???";
    }
}

int initPersistence() {
    if (ensureDirExists("GAMES") != 0 || ensureDirExists("SCORES") != 0) {
        return -1;
    }
    umask(UMASK);
    return 0;
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
        printf("number of entries: %zu", list->numEntries);
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

void destroyWordList(WordList *list) {
    if (list == NULL) {
        return;
    }
    for (size_t i = 0; i < list->numEntries; i++) {
        destroyWordListEntry(list->entries[i]);
    }
    free(list->entries);
    free(list);
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

void destroyWordListEntry(WordListEntry *entry) {
    if (entry == NULL) {
        return;
    }
    free(entry->word);
    free(entry->hintFile);
    free(entry);
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

Game *newGame(char *PLID, ServerState *state) {
    Game *game = malloc(sizeof(Game));
    if (game == NULL) {
        return NULL;
    }
    game->PLID = PLID;
    game->outcome = OUTCOME_ONGOING;
    game->numTrials = 0;
    game->trials = NULL;
    game->numSucc = 0;
    game->maskedWord = NULL;
    if ((game->wordListEntry = chooseWordListEntry(state)) == NULL) {
        destroyGame(game);
        return NULL;
    }
    game->maskedWord = strdup(game->wordListEntry->word);
    if (game->maskedWord == NULL) {
        destroyGame(game);
        return NULL;
    }
    for (size_t i = 0; i < strlen(game->maskedWord); i++) {
        game->maskedWord[i] = '_';
    }
    game->maxErrors = calculateMaxErrors(game->wordListEntry->word);
    game->remainingLetters = (unsigned int)strlen(game->wordListEntry->word);
    return game;
}

void destroyGame(Game *game) {
    if (game == NULL) {
        return;
    }
    free(game->PLID);
    free(game->maskedWord);
    for (size_t i = 0; i < game->numTrials; i++) {
        destroyGameTrial(game->trials[i]);
    }
    free(game->trials);
    free(game);
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
    if (fprintf(file, "%s %s %s\n%c %u %d %u", game->wordListEntry->word,
                game->maskedWord, game->wordListEntry->hintFile, game->outcome,
                game->numSucc, game->maxErrors, game->remainingLetters) <= 0) {
        fclose(file);
        return -1;
    }
    for (size_t i = 0; i < game->numTrials; i++) {
        if (game->trials[i]->type == TRIAL_TYPE_LETTER) {
            if (fprintf(file, "\n%c %c %d", game->trials[i]->type,
                        game->trials[i]->guess.letter,
                        game->trials[i]->correct) <= 0) {
                fclose(file);
                return -1;
            }
        } else if (game->trials[i]->type == TRIAL_TYPE_WORD) {
            if (fprintf(file, "\n%c %s %d", game->trials[i]->type,
                        game->trials[i]->guess.word,
                        game->trials[i]->correct) <= 0) {
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
    game->numTrials = 0;
    game->trials = NULL;
    game->PLID = strdup(PLID);
    game->wordListEntry = NULL;
    game->maskedWord = NULL;
    if (game->PLID == NULL) {
        destroyGame(game);
        fclose(file);
        return NULL;
    }
    char *word = NULL;
    char *hintFile = NULL;
    char outcome;
    if (fscanf(file, "%ms %ms %ms\n%c %u %d %u\n", &word, &game->maskedWord,
               &hintFile, &outcome, &game->numSucc, &game->maxErrors,
               &game->remainingLetters) != 7) {
        destroyGame(game);
        fclose(file);
        return NULL;
    }
    game->outcome = (enum GameOutcome)outcome;
    game->wordListEntry = createWordListEntry(word, hintFile);
    free(word);
    free(hintFile);
    if (game->wordListEntry == NULL) {
        destroyGame(game);
        fclose(file);
        return NULL;
    }

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char type;
    char *guess = malloc((MAX_WORD_SIZE + 1) * sizeof(char));
    if (guess == NULL) {
        destroyGame(game);
        fclose(file);
        return NULL;
    }
    int correct;
    while ((read = getline(&line, &len, file)) != -1) {
        if (sscanf(line, "%c %s %d\n", &type, guess, &correct) != 3) {
            destroyGame(game);
            fclose(file);
            free(guess);
            return NULL;
        }
        GameTrial *trial = malloc(sizeof(GameTrial));
        trial->type = type;
        if (type == TRIAL_TYPE_LETTER) {
            trial->guess.letter = guess[0];
        } else if (type == TRIAL_TYPE_WORD) {
            trial->guess.word = strdup(guess);
            if (trial->guess.word == NULL) {
                destroyGameTrial(trial);
                destroyGame(game);
                fclose(file);
                free(guess);
                return NULL;
            }
        }
        trial->correct = (bool)correct;
        if (registerGameTrial(game, trial) == -1) {
            destroyGameTrial(trial);
            destroyGame(game);
            fclose(file);
            free(guess);
            return NULL;
        }
    }

    free(line);
    free(guess);
    fclose(file);
    return game;
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
    if (file == NULL) {
        if (errno != ENOENT || ongoingOnly) {
            free(filePath);
            return NULL;
        }
        filePath = computeGameFilePath(PLID, false);
        if (filePath == NULL) {
            return NULL;
        }
        file = fopen(filePath, "r");
        if (file == NULL) {
            free(filePath);
            return NULL;
        }
    }
    free(filePath);
    return file;
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
        destroyGame(game);
        return -1;
    }
    return unlink(computeGameFilePath(game->PLID, true));
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

void destroyGameTrial(GameTrial *trial) {
    if (trial == NULL) {
        return;
    }
    if (trial->type == TRIAL_TYPE_WORD) {
        free(trial->guess.word);
    }
    free(trial);
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

unsigned int calculateScore(Game *game) {
    if (game == NULL || game->numSucc == 0 || game->numTrials == 0) {
        return 0;
    }
    // round(game->numSucc / game->numTrials)
    unsigned int rounded =
        ((100 * game->numSucc) + (game->numTrials / 2)) / game->numTrials;

    return MIN(100, rounded);
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
        free(score);
        free(word);
        return NULL;
    }
    score->word = word;

    char *fileName = strrchr(filePath, '/') + 1;
    if (fileName == NULL) {
        free(score);
        free(word);
        fclose(file);
        return NULL;
    }

    char *scoreStr = strtok(fileName, "_");
    score->PLID = strdup(strtok(NULL, "_"));
    score->finishStamp = strdup(strtok(NULL, "."));
    if (scoreStr == NULL || score->PLID == NULL || score->finishStamp == NULL ||
        sscanf(scoreStr, "%u", &score->score) != 1 ||
        fscanf(file, "%s %u %u", score->word, &score->numSucc,
               &score->numTrials) != 3) {
        destroyScore(score);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return score;
}

int generateScoreboard() {
    FILE *file = fopen(SCORES_DIR "/" SCOREBOARD_FILE_NAME, "w");
    if (file == NULL || flock(fileno(file), LOCK_EX) == -1) {
        return -1;
    }

    struct dirent **namelist;
    int n = scandir(SCORES_DIR, &namelist, isNotScoreboardFile, alphasort);
    if (n <= 0) {
        fclose(file);
        unlink(SCORES_DIR "/" SCOREBOARD_FILE_NAME);
        return n;
    }

    fprintf(file, "------------------------------- TOP 10 SCORES "
                  "-------------------------------\n\n");
    fprintf(file, "     SCORE  PLAYER  WORD                            "
                  "GOOD TRIALS  "
                  "TOTAL TRIALS\n\n");

    char filePath[MAX_FILE_PATH_SIZE];

    for (int i = n - 1; i >= MAX(0, n - SCOREBOARD_MAX_ITEMS); i--) {
        if (snprintf(filePath, MAX_FILE_PATH_SIZE, "%s/%s", SCORES_DIR,
                     namelist[i]->d_name) <= 0) {
            continue;
        }
        free(namelist[i]);
        Score *score = loadScore(filePath);
        if (score == NULL) {
            continue;
        }
        fprintf(file, "%2d - %5u  %-6s  %-30s  %11u  %12u\n", n - i,
                score->score, score->PLID, score->word, score->numSucc,
                score->numTrials);
        destroyScore(score);
    }
    free(namelist);

    fclose(file);
    return 0;
}

ResponseFile *getScoreboard() {
    char *name = malloc((MAX_FNAME_LEN + 1) * sizeof(char));
    char *stamp = formattedTimeStamp();
    if (name == NULL || stamp == NULL ||
        snprintf(name, MAX_FNAME_LEN + 1, "SB_%s.txt", stamp) <= 0) {
        free(stamp);
        free(name);
        return NULL;
    }
    free(stamp);

    return getFSResponseFile(SCORES_DIR, SCOREBOARD_FILE_NAME, name);
}

ResponseFile *getGameState(Game *game) {
    if (game == NULL) {
        return NULL;
    }

    char *fname = malloc((MAX_FNAME_LEN + 1) * sizeof(char));
    if (fname == NULL ||
        snprintf(fname, MAX_FNAME_LEN + 1, "STATE_%s.txt", game->PLID) <= 0) {
        free(fname);
        return NULL;
    }

    FILE *tmp = tmpfile();
    if (tmp == NULL) {
        free(fname);
        return NULL;
    }

    if (fprintf(tmp, "\t%s game for player %s",
                game->outcome == OUTCOME_ONGOING ? "Active" : "Last finalized",
                game->PLID) <= 0) {
        free(fname);
        fclose(tmp);
        return NULL;
    }

    if (game->outcome != OUTCOME_ONGOING &&
        fprintf(tmp, "\n\tWord: %s; Hint file: %s", game->wordListEntry->word,
                game->wordListEntry->hintFile) <= 0) {
        free(fname);
        fclose(tmp);
        return NULL;
    }

    if (game->numTrials == 0) {
        if (fprintf(tmp, "\n\n\tGame started - no trials found") <= 0) {
            free(fname);
            fclose(tmp);
            return NULL;
        }
    } else {
        if (fprintf(tmp, "\n\n\t--- Trials found: %u (%u correct) ---",
                    game->numTrials, game->numSucc) <= 0) {
            free(fname);
            fclose(tmp);
            return NULL;
        }

        for (unsigned int i = 0; i < game->numTrials; i++) {
            GameTrial *trial = game->trials[i];
            int r = 0;
            if (trial->type == TRIAL_TYPE_LETTER) {
                r = fprintf(tmp, "\n\tLetter trial: %c", trial->guess.letter);
            } else if (trial->type == TRIAL_TYPE_WORD) {
                r = fprintf(tmp, "\n\tWord guess: %s", trial->guess.word);
            }

            if (r <= 0 || fprintf(tmp, " - %s",
                                  trial->correct ? "Correct" : "Wrong") <= 0) {
                free(fname);
                fclose(tmp);
                return NULL;
            }
        }
    }

    int r = 0;
    if (game->outcome == OUTCOME_ONGOING) {
        r = fprintf(
            tmp,
            "\n\n\tGuessed so far: %s (%lu letters, %u remaining)\n\t%u "
            "wrong guesses left",
            game->maskedWord, strlen(game->maskedWord), game->remainingLetters,
            game->maxErrors - (game->numTrials - game->numSucc));
    } else {
        r = fprintf(tmp, "\n\n\tOutcome: %s",
                    translateGameOutcome(game->outcome));
    }
    if (r <= 0 || fflush(tmp) == EOF) {
        free(fname);
        fclose(tmp);
        return NULL;
    }

    rewind(tmp);
    return getResponseFile(tmp, fname);
}

// Closes file if not NULL; takes ownership of responseFileName
ResponseFile *getResponseFile(FILE *file, char *responseFileName) {
    if (file == NULL) {
        free(responseFileName);
        return NULL;
    }

    if (responseFileName == NULL || flock(fileno(file), LOCK_SH) == -1) {
        free(responseFileName);
        fclose(file);
        return NULL;
    }

    struct stat fileStat;
    if (fstat(fileno(file), &fileStat) == -1) {
        free(responseFileName);
        fclose(file);
        return NULL;
    }

    ResponseFile *resp = malloc(sizeof(ResponseFile));
    if (resp == NULL) {
        free(responseFileName);
        fclose(file);
        return NULL;
    }

    if (fileStat.st_size > MAX_FSIZE_NUM) {
        free(responseFileName);
        free(resp);
        fclose(file);
        return NULL;
    }
    resp->size = (size_t)fileStat.st_size;

    resp->name = responseFileName;
    resp->data = malloc(resp->size);
    if (resp->data == NULL) {
        destroyResponseFile(resp);
        fclose(file);
        return NULL;
    }

    if (fread(resp->data, sizeof(char), resp->size, file) < resp->size) {
        destroyResponseFile(resp);
        fclose(file);
        return NULL;
    }

    fclose(file);
    return resp;
}

void destroyResponseFile(ResponseFile *resp) {
    if (resp == NULL) {
        return;
    }
    free(resp->name);
    free(resp->data);
    free(resp);
}

ResponseFile *getFSResponseFile(char *dirPath, char *fileName,
                                char *responseFileName) {
    if (dirPath == NULL || fileName == NULL) {
        return NULL;
    }
    char *filePath = malloc(MAX_FILE_PATH_SIZE * sizeof(char));
    if (filePath == NULL ||
        sprintf(filePath, "%s/%s", dirPath, fileName) <= 0) {
        free(filePath);
        free(responseFileName);
        return NULL;
    }
    if (responseFileName == NULL) {
        responseFileName = strndup(fileName, MAX_FNAME_LEN);
    }

    FILE *file = fopen(filePath, "r");
    free(filePath);

    return getResponseFile(file, responseFileName);
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