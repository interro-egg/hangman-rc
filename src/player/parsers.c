#include "parsers.h"
#include <stdlib.h>

SNGMessage *parseSNGArgs(char *args){
    SNGMessage *sng = malloc(sizeof(SNGMessage));
    if (sscanf(args, "%m6s", &sng->PLID) != 1){
        return NULL;
    }
    return sng;
}

PLGMessage *parsePLGArgs(char *args){
    PLGMessage *plg = malloc(sizeof(PLGMessage));
    if (sscanf(args, "%1c", &plg->letter) != 1){
        return NULL;
    }
    return plg;
}

PWGMessage *parsePWGArgs(char *args){
    PWGMessage *pwg = malloc(sizeof(PWGMessage));
    if (sscanf(args, "%m30s", &pwg->word) != 1){
        return NULL;
    }
    return pwg;
}
