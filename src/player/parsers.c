#include "parsers.h"
#include "../common/messages.h"
#include <errno.h>
#include <stdlib.h>

SNGMessage *parseSNGArgs(char *args) {
    SNGMessage *sng = malloc(sizeof(SNGMessage));
    sng-> PLID = malloc(7 * sizeof(char));
    if (sng == NULL || sng->PLID == NULL) {
        errno = ENOMEM;
        destroySNGMessage(sng);
        return NULL;
    }
    if (sscanf(args, "%6s", sng->PLID) != 1) {
        destroySNGMessage(sng);
        return NULL;
    }
    return sng;
}

PLGMessage *parsePLGArgs(char *args) {
    PLGMessage *plg = malloc(sizeof(PLGMessage));
    if (plg == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    if (sscanf(args, "%1c", &plg->letter) != 1) {
        destroyPLGMessage(plg);
        return NULL;
    }
    return plg;
}

PWGMessage *parsePWGArgs(char *args) {
    PWGMessage *pwg = malloc(sizeof(PWGMessage));
    pwg->word = malloc(31 * sizeof(char));
    if (pwg == NULL || pwg->word == NULL) {
        errno = ENOMEM;
        destroyPWGMessage(pwg);
        return NULL;
    }
    if (sscanf(args, "%30s", &pwg->word) != 1) {
        destroyPWGMessage(pwg);
        return NULL;
    }
    return pwg;
}
