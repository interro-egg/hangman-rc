#include "persistence.h"
#include <stdlib.h>

Game *newGame(char *PLID, ServerState *state) {
    Game *game = malloc(sizeof(Game));
    if (game == NULL) {
        return NULL;
    }
    game->__curFilePath = NULL;
    game->PLID = PLID;
    game->outcome = OUTCOME_ONGOING;
    game->finishStamp = NULL;
    WordList *list = getWordList(state);
    if (list == NULL ||
        (game->wordListEntry = chooseWordListEntry(list, state)) == NULL) {
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