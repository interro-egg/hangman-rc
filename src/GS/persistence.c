#include "persistence.h"
#include "../common/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Game *newGame(char *PLID, ServerState *state) {
    Game *game = malloc(sizeof(Game));
    if (game == NULL) {
        return NULL;
    }
    game->__curFilePath = NULL;
    game->PLID = PLID;
    game->outcome = OUTCOME_ONGOING;
    game->finishStamp = NULL;
    WordList *list = state->word_list;
    if ((game->wordListEntry = chooseWordListEntry(list, state)) == NULL) {
        destroyWordList(list);
        destroyWordListEntry(game->wordListEntry);
        return NULL;
    }
    game->numTrials = 0;
    game->trials = NULL;
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
    free(game->trials);
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
        destroyWordListEntry(&list->entries[i]);
    }
    free(list->entries);
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

WordListEntry *chooseWordListEntry(WordList *list, ServerState *state) {
    if (list == NULL) {
        return NULL;
    }
    if (state->randomize_word_list) {
        return chooseRandomWordListEntry(list);
    } else {
        return chooseSequentialWordListEntry(list);
    }
}

WordListEntry *chooseRandomWordListEntry(WordList *list) {
    if (list == NULL) {
        return NULL;
    }
    if (list->numEntries == 0) {
        return NULL;
    }
    size_t index = (size_t)rand() % list->numEntries;
    return &list->entries[index];
}

WordListEntry *chooseSequentialWordListEntry(UNUSED WordList *list) {
    // ???
    return NULL;
}

WordListEntry *generateWordListEntry(char *word, char *hintFile) {
    WordListEntry *entry = malloc(sizeof(WordListEntry));
    if (entry == NULL) {
        return NULL;
    }
    entry->word = word;
    entry->hintFile = hintFile;
    return entry;
}

WordList *generateWordList(char *wordFile) {
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
        char *word = strtok(line, " ");
        char *hintFile = strtok(NULL, " ");
        if (word == NULL || hintFile == NULL) {
            return NULL;
        }
        WordListEntry *entry = generateWordListEntry(word, hintFile);
        if (entry == NULL) {
            return NULL;
        }
        list->numEntries++;
        list->entries =
            realloc(list->entries, sizeof(WordListEntry) * list->numEntries);
        if (list->entries == NULL) {
            return NULL;
        }
        list->entries[list->numEntries - 1] = *entry;
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
    game->trials = realloc(game->trials, sizeof(GameTrial) * game->numTrials);
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
    char *filePath = computeGameFilePath(game);
    if (filePath == NULL) {
        return -1;
    }
    FILE *file = fopen(filePath, "w");
    if (file == NULL) {
        return -1;
    }
    // ???? cenas
    return 0;
}

char *computeGameFilePath(UNUSED Game *game) { return NULL; }