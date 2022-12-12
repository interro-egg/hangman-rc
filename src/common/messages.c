#include "messages.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>

void destroySNGMessage(SNGMessage *msg) {
    free(msg->PLID);
    free(msg);
}

ssize_t serializeSNGMessage(SNGMessage *msg, char *outBuffer) {
    return sprintf(outBuffer, "SNG %s\n", msg->PLID);
}

SNGMessage *deserializeSNGMessage(char *inBuffer) {
    SNGMessage *msg = malloc(sizeof(SNGMessage));
    msg->PLID = malloc(sizeof(char) * 7);
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

void destroyRSGMessage(RSGMessage *msg) { free(msg); }

ssize_t serializeRSGMessage(RSGMessage *msg, char *outBuffer) {
    return sprintf(outBuffer, "RSG %s %u %u\n",
                   RSGMessageStatusStrings[msg->status], msg->n_letters,
                   msg->max_errors);
}

RSGMessage *deserializeRSGMessage(char *inBuffer) {
    RSGMessage *msg = malloc(sizeof(RSGMessage));
    char *status = malloc(sizeof(char) * 3);
    if (msg == NULL || status == NULL) {
        destroyRSGMessage(msg);
        free(status);
        return NULL;
    }
    if (sscanf(inBuffer, "%3s", status) != 1) {
        destroyRSGMessage(msg);
        free(status);
        return NULL;
    }
    // TODO: use RSGMessageStatusStrings??
    if (strcmp(status, "OK") == 0) {
        msg->status = RSG_OK;
        inBuffer += 2;
    } else if (strcmp(status, "NOK") == 0) {
        msg->status = RSG_NOK;
        inBuffer += 3;
    } else {
        destroyRSGMessage(msg);
        free(status);
        return NULL;
    }
    free(status);
    if (readUnsignedInt(inBuffer, &msg->n_letters) != 0 ||
        readUnsignedInt(inBuffer, &msg->max_errors) != 0) {
        destroyRSGMessage(msg);
        return NULL;
    }
    return msg;
}

void destroyPLGMessage(PLGMessage *msg) {
    free(msg->PLID);
    free(msg);
}

ssize_t serializePLGMessage(PLGMessage *msg, char *outBuffer) {
    return sprintf(outBuffer, "PLG %s %c %u\n", msg->PLID, msg->letter,
                   msg->trial);
}

PLGMessage *deserializePLGMessage(char *inBuffer) {
    PLGMessage *msg = malloc(sizeof(PLGMessage));
    msg->PLID = malloc(sizeof(char) * 7);
    if (msg == NULL || msg->PLID == NULL) {
        destroyPLGMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "%6s %1c %1u", msg->PLID, msg->letter, msg->trial) !=
        3) {
        destroyPLGMessage(msg);
        return NULL;
    }
    return msg;
}

void destroyRLGMessage(RLGMessage *msg) { free(msg); }

ssize_t serializeRLGMessage(RLGMessage *msg, char *outBuffer) {
    return sprintf(outBuffer, "RLG %s %u %u %u\n", msg->status, msg->trial,
                   msg->n, msg->pos);
}

RLGMessage *deserializeRLGMessage(char *inBuffer) { return NULL; }

void destroyPWGMessage(PWGMessage *msg) { return NULL; }

ssize_t serializePWGMessage(PWGMessage *msg, char *outBuffer) { return NULL; }

PWGMessage *deserializePWGMessage(char *inBuffer) { return NULL; }

void destroyRWGMessage(RWGMessage *msg) { return NULL; }

ssize_t serializeRWGMessage(RWGMessage *msg, char *outBuffer) { return NULL; }

RWGMessage *deserializeRWGMessage(char *inBuffer) { return NULL; }

void destroyQUTMessage(QUTMessage *msg) { return NULL; }

ssize_t serializeQUTMessage(QUTMessage *msg, char *outBuffer) { return NULL; }

QUTMessage *deserializeQUTMessage(char *inBuffer) { return NULL; }

void destroyRQTMessage(RQTMessage *msg) { return NULL; }

ssize_t serializeRQTMessage(RQTMessage *msg, char *outBuffer) { return NULL; }

RQTMessage *deserializeRQTMessage(char *inBuffer) { return NULL; }

void destroyREVMessage(REVMessage *msg) { return NULL; }

ssize_t serializeREVMessage(REVMessage *msg, char *outBuffer) { return NULL; }

REVMessage *deserializeREVMessage(char *inBuffer) { return NULL; }

void destroyRRVMessage(RRVMessage *msg) {
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
    if (msg == NULL) {
        return NULL;
    }
    int status = parse_enum(RRVMessageStatusStrings, inBuffer);
    if (status != -1) {
        msg->type = RRV_STATUS;
        msg->word = NULL;
        msg->status = status;
    } else {
        msg->type = RRV_WORD;
        if (sscanf("%m30s", msg->word) != 1) {
            destroyRRVMessage(msg);
            return NULL;
        }
        msg->status = NULL;
    }
    return msg;
}