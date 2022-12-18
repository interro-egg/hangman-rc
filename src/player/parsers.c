#include "parsers.h"
#include "../common/messages.h"
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

void *parseSNGArgs(char *args) {
    if (args == NULL || strlen(args) != 6) {
        return NULL;
    }

    SNGMessage *sng = malloc(sizeof(SNGMessage));
    sng->PLID = malloc(7 * sizeof(char));
    if (sng == NULL || sng->PLID == NULL) {
        errno = ENOMEM;
        destroySNGMessage(sng);
        return NULL;
    }

    if (sscanf(args, "%6s", sng->PLID) != 1) {
        destroySNGMessage(sng);
        return NULL;
    }

    size_t len = strlen(sng->PLID);
    for (size_t i = 0; i < len; i++) {
        if (!isdigit(sng->PLID[i])) {
            destroySNGMessage(sng);
            return NULL;
        }
    }
    return sng;
}

void *parsePLGArgs(char *args) {
    if (args == NULL || strlen(args) != 1) {
        return NULL;
    }

    PLGMessage *plg = malloc(sizeof(PLGMessage));
    plg->PLID = malloc(7 * sizeof(char));
    if (plg == NULL || plg->PLID == NULL) {
        errno = ENOMEM;
        return NULL;
    }

    if (sscanf(args, "%1c", &plg->letter) != 1 || !isalpha(plg->letter)) {
        destroyPLGMessage(plg);
        return NULL;
    }
    return plg;
}

void *parsePWGArgs(char *args) {
    if (args == NULL) {
        return NULL;
    }

    PWGMessage *pwg = malloc(sizeof(PWGMessage));
    pwg->PLID = malloc(7 * sizeof(char));
    pwg->word = malloc(31 * sizeof(char));
    if (pwg == NULL || pwg->PLID == NULL || pwg->word == NULL) {
        errno = ENOMEM;
        destroyPWGMessage(pwg);
        return NULL;
    }

    if (sscanf(args, "%30s", pwg->word) != 1) {
        destroyPWGMessage(pwg);
        return NULL;
    }

    size_t len = strlen(pwg->word);
    if (len < strlen(args)) {
        destroyPWGMessage(pwg);
        return NULL;
    }

    for (size_t i = 0; i < len; i++) {
        if (!isalpha(pwg->word[i])) {
            destroyPWGMessage(pwg);
            return NULL;
        }
    }

    return pwg;
}

void *parseQUTArgs(char *args) {
    if (args != NULL) {
        return NULL;
    }
    QUTMessage *qut = malloc(sizeof(QUTMessage));
    qut->PLID = malloc(7 * sizeof(char));
    if (qut == NULL || qut->PLID == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    return qut;
}

void *parseREVArgs(char *args) {
    if (args != NULL) {
        return NULL;
    }
    REVMessage *rev = malloc(sizeof(REVMessage));
    rev->PLID = malloc(7 * sizeof(char));
    if (rev == NULL || rev->PLID == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    return rev;
}

void *parseGSBArgs(char *args) {
    if (args != NULL) {
        return NULL;
    }
    GSBMessage *gsb = malloc(sizeof(GSBMessage));
    return gsb;
}

void *parseGHLArgs(char *args) {
    if (args != NULL) {
        return NULL;
    }
    GHLMessage *ghl = malloc(sizeof(GHLMessage));
    ghl->PLID = malloc(7 * sizeof(char));
    if (ghl == NULL || ghl->PLID == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    return ghl;
}

void *parseSTAArgs(char *args) {
    if (args != NULL) {
        return NULL;
    }
    STAMessage *sta = malloc(sizeof(STAMessage));
    sta->PLID = malloc(7 * sizeof(char));
    if (sta == NULL || sta->PLID == NULL) {
        errno = ENOMEM;
        return NULL;
    }
    return sta;
}