#include "persistence.h"
#include "../common/common.h"
#include <errno.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <unistd.h>

Game *newGame(char *PLID, ServerState *state) {
    Game *game = malloc(sizeof(Game));
    if (game == NULL) {
        return NULL;
    }
    game->PLID = PLID;
    game->outcome = OUTCOME_ONGOING;
    game->finishStamp = NULL;
    if ((game->wordListEntry = chooseWordListEntry(state)) == NULL) {
        return NULL;
    }
    game->numTrials = 0;
    game->trials = NULL;
    game->numSucc = 0;
    game->maxErrors = calculateMaxErrors(game->wordListEntry->word);
    return game;
}

void destroyGame(Game *game) {
    if (game == NULL) {
        return;
    }
    free(game->PLID);
    free(game->finishStamp);
    destroyWordListEntry(game->wordListEntry);
    for (size_t i = 0; i < game->numTrials; i++) {
        destroyGameTrial(&game->trials[i]);
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
    return list->entries[(*seqPtr)++]; //🤯
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
        printf("word: [%s], hintFile: [%s]\n", word, hintFile);
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
        reallocarray(game->trials, game->numTrials, sizeof(GameTrial));
    if (game->trials == NULL) {
        return -1;
    }
    game->trials[game->numTrials - 1] = *trial;
    return 0;
}

int saveGame(Game *game, UNUSED ServerState *state) {
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
    if (fprintf(file, "%s %s\n%c %lu %d\n", game->wordListEntry->word,
                game->wordListEntry->hintFile, game->outcome, game->numSucc,
                game->maxErrors) <= 0) {
        fclose(file);
        return -1;
    }
    for (size_t i = 0; i < game->numTrials; i++) {
        if (game->trials[i].type == TRIAL_TYPE_LETTER) {
            fprintf(file, "%c %c\n", game->trials[i].type,
                    game->trials[i].guess.letter);
        } else if (game->trials[i].type == TRIAL_TYPE_WORD) {
            fprintf(file, "%c %s\n", game->trials[i].type,
                    game->trials[i].guess.word);
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
    if (fscanf(file, "%s %s\n%c %lu %d\n", word, hintFile,
               (char *)&game->outcome, &game->numSucc, &game->maxErrors) != 5) {
        free(game);
        fclose(file);
        return NULL;
    }
    game->wordListEntry = createWordListEntry(word, hintFile);
    if (game->wordListEntry == NULL) {
        free(game);
        fclose(file);
        return NULL;
    }
    game->numTrials = 0;
    game->trials = NULL;

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char type;
    char *guess = malloc(sizeof(char) * MAX_GUESS_SIZE);
    while ((read = getline(&line, &len, file)) != -1) {
        if (sscanf(line, "%c %s\n", &type, guess) != 2) {
            free(game);
            fclose(file);
            return NULL;
        }
        GameTrial trial;
        trial.type = type;
        if (type == TRIAL_TYPE_LETTER) {
            trial.guess.letter = guess[0];
        } else if (type == TRIAL_TYPE_WORD) {
            trial.guess.word = guess;
        }
        if (registerGameTrial(game, &trial) == -1) {
            free(game);
            fclose(file);
            return NULL;
        }
    }

    free(line);
    fclose(file);
    return game;
}

unsigned long getScore(Game *game) {
    if (game == NULL || game->numTrials == 0) {
        return 0;
    }
    // round(game->numSucc / game->numTrials)
    return (game->numSucc + (game->numTrials / 2)) / game->numTrials;
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
            free(filePath);
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