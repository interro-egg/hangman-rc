#include "messages.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>

void destroySNGMessage(SNGMessage *msg) {
    if (msg != NULL)
        free(msg->PLID);
    free(msg);
}

ssize_t serializeSNGMessage(SNGMessage *msg, char *outBuffer) {
    return sprintf(outBuffer, "SNG %s\n", msg->PLID);
}

SNGMessage *deserializeSNGMessage(char *inBuffer) {
    SNGMessage *msg = malloc(sizeof(SNGMessage));
    msg->PLID = malloc(7 * sizeof(char));
    if (msg == NULL || msg->PLID == NULL) {
        destroySNGMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "%6s", msg->PLID) != 1) {
        destroySNGMessage(msg);
        return NULL;
    }
    return msg;
}

const char *RSGMessageStatusStrings[] = {"OK", "NOK"};

void destroyRSGMessage(RSGMessage *msg) { free(msg); }

ssize_t serializeRSGMessage(RSGMessage *msg, char *outBuffer) {
    return sprintf(outBuffer, "RSG %s %u %u\n",
                   RSGMessageStatusStrings[msg->status], &msg->n_letters,
                   &msg->max_errors);
}

RSGMessage *deserializeRSGMessage(char *inBuffer) {
    RSGMessage *msg = malloc(sizeof(RSGMessage));
    char *statusStr = malloc(4 * sizeof(char));
    if (msg == NULL || statusStr == NULL) {
        destroyRSGMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "%3s", statusStr) != 1) {
        destroyRSGMessage(msg);
        return NULL;
    }
    int status = parseEnum(RSGMessageStatusStrings, statusStr);
    if (status == -1) {
        destroyRSGMessage(msg);
        free(statusStr);
        return NULL;
    }
    msg->status = status;
    free(statusStr);

    if (sscanf(inBuffer, "%*s %2u %1u", &msg->n_letters, &msg->max_errors) !=
        2) {
        destroyRSGMessage(msg);
        return NULL;
    }
    return msg;
}

void destroyPLGMessage(PLGMessage *msg) {
    if (msg != NULL)
        free(msg->PLID);
    free(msg);
}

ssize_t serializePLGMessage(PLGMessage *msg, char *outBuffer) {
    return sprintf(outBuffer, "PLG %s %c %u\n", msg->PLID, &msg->letter,
                   &msg->trial);
}

PLGMessage *deserializePLGMessage(char *inBuffer) {
    PLGMessage *msg = malloc(sizeof(PLGMessage));
    msg->PLID = malloc(7 * sizeof(char));
    if (msg == NULL || msg->PLID == NULL) {
        destroyPLGMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "%6s %1c %2u", msg->PLID, &msg->letter, &msg->trial) !=
        3) {
        destroyPLGMessage(msg);
        return NULL;
    }
    return msg;
}

const char *RLGMessageStatusStrings[] = {"OK",  "WIN", "DUP", "NOK",
                                         "OVR", "INV", "ERR"};

void destroyRLGMessage(RLGMessage *msg) {
    if (msg != NULL)
        free(msg->pos);
    free(msg);
}

ssize_t serializeRLGMessage(RLGMessage *msg, char *outBuffer) {
    char *posBuf = malloc((msg->n * 3 + 1) * sizeof(char));
    char *cur = posBuf;
    if (posBuf == NULL)
        return -1;

    for (unsigned int i = 0; i < msg->n; i++) {
        int r = sprintf(cur, " %u", msg->pos[i]);
        if (r < 0) {
            free(posBuf);
            return -1;
        }
        cur += r;
    }

    if (sprintf(outBuffer, "RLG %s %u %u %s",
                RLGMessageStatusStrings[msg->status], &msg->trial, &msg->n,
                posBuf) < 0) {
        free(posBuf);
        return -1;
    }
    return 0;
}

RLGMessage *deserializeRLGMessage(char *inBuffer) {
    RLGMessage *msg = malloc(sizeof(RLGMessage));
    char *statusStr = malloc(4 * sizeof(char));
    if (msg == NULL || statusStr == NULL) {
        destroyRLGMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "%3s", statusStr) != 1) {
        destroyRLGMessage(msg);
        return NULL;
    }
    int status = parseEnum(RLGMessageStatusStrings, statusStr);
    if (status == -1) {
        destroyRLGMessage(msg);
        free(statusStr);
        return NULL;
    }
    msg->status = status;
    free(statusStr);
    if (sscanf(inBuffer, "%*s %2u %2u", &msg->trial, &msg->n) != 2) {
        destroyRLGMessage(msg);
        return NULL;
    }
    msg->pos = malloc(msg->n * sizeof(unsigned int));
    if (msg->pos == NULL) {
        destroyRLGMessage(msg);
        return NULL;
    }

    char *cur = strtok(inBuffer, " ");
    for (unsigned int i = 0; cur != NULL; i++) {
        if (i > 1) {
            if (sscanf(cur, "%2u", msg->pos[i - 2]) != 1) {
                destroyRLGMessage(msg);
                return NULL;
            }
        }
        cur = strtok(NULL, " ");
    }

    return msg;
}

void destroyPWGMessage(PWGMessage *msg) {
    if (msg != NULL) {
        free(msg->PLID);
        free(msg->word);
    }
    free(msg);
}

ssize_t serializePWGMessage(PWGMessage *msg, char *outBuffer) {
    return sprintf(outBuffer, "PWG %s %s %u\n", msg->PLID, msg->word,
                   &msg->trial);
}

PWGMessage *deserializePWGMessage(char *inBuffer) {
    PWGMessage *msg = malloc(sizeof(PWGMessage));
    msg->PLID = malloc(7 * sizeof(char));
    msg->word = malloc(31 * sizeof(char));
    if (msg == NULL || msg->PLID == NULL || msg->word == NULL) {
        destroyPWGMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "%6s %30s %2u", msg->PLID, msg->word, &msg->trial) !=
        3) {
        destroyPWGMessage(msg);
        return NULL;
    }
    return msg;
}

const char *RWGMessageStatusStrings[] = {"WIN", "NOK", "OVR", "INV", "ERR"};

void destroyRWGMessage(RWGMessage *msg) { free(msg); }

ssize_t serializeRWGMessage(RWGMessage *msg, char *outBuffer) {
    return sprintf(outBuffer, "RWG %s %u\n",
                   RWGMessageStatusStrings[msg->status], &msg->trials);
}

RWGMessage *deserializeRWGMessage(char *inBuffer) {
    RWGMessage *msg = malloc(sizeof(RWGMessage));
    char *statusStr = malloc(4 * sizeof(char));
    if (msg == NULL || statusStr == NULL) {
        destroyRWGMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "%3s", statusStr) != 1) {
        destroyRWGMessage(msg);
        return NULL;
    }
    int status = parseEnum(RWGMessageStatusStrings, statusStr);
    if (status == -1) {
        destroyRWGMessage(msg);
        free(statusStr);
        return NULL;
    }
    msg->status = status;
    free(statusStr);
    if (sscanf(inBuffer, "%*s %2u", &msg->trials) != 1) {
        destroyRWGMessage(msg);
        return NULL;
    }
    return msg;
}

void destroyQUTMessage(QUTMessage *msg) {
    if (msg != NULL)
        free(msg->PLID);
    free(msg);
}

ssize_t serializeQUTMessage(QUTMessage *msg, char *outBuffer) {
    return sprintf(outBuffer, "QUT %s\n", msg->PLID);
}

QUTMessage *deserializeQUTMessage(char *inBuffer) {
    QUTMessage *msg = malloc(sizeof(QUTMessage));
    msg->PLID = malloc(7 * sizeof(char));
    if (msg == NULL || msg->PLID == NULL) {
        destroyQUTMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "%6s", msg->PLID) != 1) {
        destroyQUTMessage(msg);
        return NULL;
    }
    return msg;
}

const char *RQTMessageStatusStrings[] = {"OK", "ERR"};

void destroyRQTMessage(RQTMessage *msg) { free(msg); }

ssize_t serializeRQTMessage(RQTMessage *msg, char *outBuffer) {
    return sprintf(outBuffer, "RQT %s\n", RQTMessageStatusStrings[msg->status]);
}

RQTMessage *deserializeRQTMessage(char *inBuffer) {
    RQTMessage *msg = malloc(sizeof(RQTMessage));
    char *statusStr = malloc(4 * sizeof(char));
    if (msg == NULL || statusStr == NULL) {
        destroyRQTMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "%3s", statusStr) != 1) {
        destroyRQTMessage(msg);
        return NULL;
    }
    int status = parseEnum(RQTMessageStatusStrings, statusStr);
    if (status == -1) {
        destroyRQTMessage(msg);
        free(statusStr);
        return NULL;
    }
    msg->status = status;
    free(statusStr);
    return msg;
}

void destroyREVMessage(REVMessage *msg) { free(msg); }

ssize_t serializeREVMessage(REVMessage *msg, char *outBuffer) {
    return sprintf(outBuffer, "REV %s\n", msg->PLID);
}

REVMessage *deserializeREVMessage(char *inBuffer) {
    REVMessage *msg = malloc(sizeof(REVMessage));
    msg->PLID = malloc(7 * sizeof(char));
    if (msg == NULL || msg->PLID == NULL) {
        destroyREVMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "%6s", msg->PLID) != 1) {
        destroyREVMessage(msg);
        return NULL;
    }
    return msg;
}

const char *RRVMessageStatusStrings[] = {"OK", "ERR"};

void destroyRRVMessage(RRVMessage *msg) {
    if (msg != NULL)
        free(msg->word);
    free(msg);
}

ssize_t serializeRRVMessage(RRVMessage *msg, char *outBuffer) {
    return sprintf(outBuffer, "RRV %s",
                   msg->type == RRV_STATUS
                       ? RRVMessageStatusStrings[msg->status]
                       : msg->word);
}

RRVMessage *deserializeRRVMessage(char *inBuffer) {
    RRVMessage *msg = malloc(sizeof(RRVMessage));
    msg->word = malloc(31 * sizeof(char));
    if (msg == NULL || msg->word == NULL) {
        return NULL;
    }
    int status = parseEnum(RRVMessageStatusStrings, inBuffer);
    if (status != -1) {
        msg->type = RRV_STATUS;
        msg->word = NULL;
        msg->status = status;
    } else {
        msg->type = RRV_WORD;
        if (sscanf("%30s", msg->word) != 1) {
            destroyRRVMessage(msg);
            return NULL;
        }
    }
    return msg;
}