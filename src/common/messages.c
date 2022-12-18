#include "messages.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>

void destroySNGMessage(void *ptr) {
    SNGMessage *msg = (SNGMessage *)ptr;
    if (msg != NULL)
        free(msg->PLID);
    free(msg);
}

ssize_t serializeSNGMessage(void *ptr, char *outBuffer) {
    SNGMessage *msg = (SNGMessage *)ptr;
    return sprintf(outBuffer, "SNG %s\n", msg->PLID);
}

void *deserializeSNGMessage(char *inBuffer) {
    SNGMessage *msg = malloc(sizeof(SNGMessage));
    if (msg == NULL) {
        destroySNGMessage(msg);
        return NULL;
    }
    msg->PLID = malloc(7 * sizeof(char));
    if (msg->PLID == NULL) {
        destroySNGMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "SNG %6s\n", msg->PLID) != 1) {
        destroySNGMessage(msg);
        return NULL;
    }
    return msg;
}

const char *RSGMessageStatusStrings[] = {"OK", "NOK", "ERR", NULL};

void destroyRSGMessage(void *ptr) {
    RSGMessage *msg = (RSGMessage *)ptr;
    free(msg);
}

ssize_t serializeRSGMessage(void *ptr, char *outBuffer) {
    RSGMessage *msg = (RSGMessage *)ptr;
    switch (msg->status) {
    case RSG_OK:
        return sprintf(outBuffer, "RSG %s %u %u\n",
                       RSGMessageStatusStrings[msg->status], msg->n_letters,
                       msg->max_errors);
    case RSG_NOK:
    case RSG_ERR:
    default:
        return sprintf(outBuffer, "RSG %s",
                       RSGMessageStatusStrings[msg->status]);
    }
}

void *deserializeRSGMessage(char *inBuffer) {
    RSGMessage *msg = malloc(sizeof(RSGMessage));
    char *statusStr = malloc(4 * sizeof(char));
    if (msg == NULL || statusStr == NULL) {
        destroyRSGMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "RSG %3s\n", statusStr) != 1) {
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
    switch (msg->status) {
    case RSG_OK:
        if (sscanf(inBuffer, "RSG %*s %2u %1u", &msg->n_letters,
                   &msg->max_errors) != 2) {
            destroyRSGMessage(msg);
            return NULL;
        }
        break;
    case RSG_NOK:
    case RSG_ERR:
    default:
        break;
    }
    return msg;
}

void destroyPLGMessage(void *ptr) {
    PLGMessage *msg = (PLGMessage *)ptr;
    if (msg != NULL)
        free(msg->PLID);
    free(msg);
}

ssize_t serializePLGMessage(void *ptr, char *outBuffer) {
    PLGMessage *msg = (PLGMessage *)ptr;
    return sprintf(outBuffer, "PLG %s %c %u\n", msg->PLID, msg->letter,
                   msg->trial);
}

void *deserializePLGMessage(char *inBuffer) {
    PLGMessage *msg = malloc(sizeof(PLGMessage));
    if (msg == NULL) {
        destroyPLGMessage(msg);
        return NULL;
    }
    msg->PLID = malloc(7 * sizeof(char));
    if (msg->PLID == NULL) {
        destroyPLGMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "PLG %6s %1c %2u\n", msg->PLID, &msg->letter,
               &msg->trial) != 3) {
        destroyPLGMessage(msg);
        return NULL;
    }
    return msg;
}

const char *RLGMessageStatusStrings[] = {"OK",  "WIN", "DUP", "NOK",
                                         "OVR", "INV", "ERR", NULL};

void destroyRLGMessage(void *ptr) {
    RLGMessage *msg = (RLGMessage *)ptr;
    if (msg != NULL && msg->pos != NULL)
        free(msg->pos);
    free(msg);
}

ssize_t serializeRLGMessage(void *ptr, char *outBuffer) {
    RLGMessage *msg = (RLGMessage *)ptr;
    char *posBuf = malloc((msg->n * 3 + 1) * sizeof(char));
    char *cur = posBuf;
    if (posBuf == NULL)
        return -1;
    switch (msg->status) {
    case RLG_OK:
        for (unsigned int i = 0; i < msg->n; i++) {
            int r = sprintf(cur, " %u", msg->pos[i]);
            if (r < 0) {
                free(posBuf);
                return -1;
            }
            cur += r;
        }

        if (sprintf(outBuffer, "RLG %s %u %u %s\n",
                    RLGMessageStatusStrings[msg->status], msg->trial, msg->n,
                    posBuf) < 0) {
            free(posBuf);
            return -1;
        }
        break;
    case RLG_WIN:
    case RLG_DUP:
    case RLG_NOK:
    case RLG_OVR:
    case RLG_INV:
        if (sprintf(outBuffer, "RLG %s %u\n",
                    RLGMessageStatusStrings[msg->status], msg->trial) < 0) {
            free(posBuf);
            return -1;
        }
        break;
    case RLG_ERR:
    default:
        if (sprintf(outBuffer, "RLG ERR\n") < 0) {
            free(posBuf);
            return -1;
        }
        break;
    }
    return 0;
}

void *deserializeRLGMessage(char *inBuffer) {
    RLGMessage *msg = malloc(sizeof(RLGMessage));
    char *statusStr = malloc(4 * sizeof(char));
    if (msg == NULL || statusStr == NULL) {
        destroyRLGMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "RLG %3s", statusStr) != 1) {
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
    msg->pos = NULL;
    switch (msg->status) {
    case RLG_OK:
        if (sscanf(inBuffer, "RLG %*s %2u %2u\n", &msg->trial, &msg->n) != 2) {
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
            if (i > 3) {
                if (sscanf(cur, "%2u", &msg->pos[i - 4]) != 1) {
                    destroyRLGMessage(msg);
                    return NULL;
                }
            }
            cur = strtok(NULL, " ");
        }
        break;
    case RLG_WIN:
    case RLG_DUP:
    case RLG_NOK:
    case RLG_OVR:
    case RLG_INV:
        if (sscanf(inBuffer, "RLG %*s %2u\n", &msg->trial) != 1) {
            destroyRLGMessage(msg);
            return NULL;
        }
        break;
    case RLG_ERR:
    default:
        if (strcmp(inBuffer, "RLG ERR\n") != 0) {
            destroyRLGMessage(msg);
            return NULL;
        }
        break;
    }
    return msg;
}

void destroyPWGMessage(void *ptr) {
    PWGMessage *msg = (PWGMessage *)ptr;
    if (msg != NULL) {
        free(msg->PLID);
        free(msg->word);
    }
    free(msg);
}

ssize_t serializePWGMessage(void *ptr, char *outBuffer) {
    PWGMessage *msg = (PWGMessage *)ptr;
    return sprintf(outBuffer, "PWG %s %s %u\n", msg->PLID, msg->word,
                   msg->trial);
}

void *deserializePWGMessage(char *inBuffer) {
    PWGMessage *msg = malloc(sizeof(PWGMessage));
    if (msg == NULL) {
        destroyPWGMessage(msg);
        return NULL;
    }
    msg->PLID = malloc(7 * sizeof(char));
    msg->word = malloc(31 * sizeof(char));
    if (msg->PLID == NULL || msg->word == NULL) {
        destroyPWGMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "PWG %6s %30s %2u\n", msg->PLID, msg->word,
               &msg->trial) != 3) {
        destroyPWGMessage(msg);
        return NULL;
    }
    return msg;
}

const char *RWGMessageStatusStrings[] = {"WIN", "DUP", "NOK", "OVR",
                                         "INV", "ERR", NULL};

void destroyRWGMessage(void *ptr) {
    RWGMessage *msg = (RWGMessage *)ptr;
    free(msg);
}

ssize_t serializeRWGMessage(void *ptr, char *outBuffer) {
    RWGMessage *msg = (RWGMessage *)ptr;
    switch (msg->status) {
    case RWG_WIN:
    case RWG_DUP:
    case RWG_NOK:
    case RWG_OVR:
    case RWG_INV:
        return sprintf(outBuffer, "RWG %s %u\n",
                       RWGMessageStatusStrings[msg->status], msg->trials);
    case RWG_ERR:
    default:
        return sprintf(outBuffer, "RWG ERR\n");
    }
}

void *deserializeRWGMessage(char *inBuffer) {
    RWGMessage *msg = malloc(sizeof(RWGMessage));
    char *statusStr = malloc(4 * sizeof(char));
    if (msg == NULL || statusStr == NULL) {
        destroyRWGMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "RWG %3s", statusStr) != 1) {
        destroyRWGMessage(msg);
        free(statusStr);
        printf("Error scanning status\n");
        return NULL;
    }
    int status = parseEnum(RWGMessageStatusStrings, statusStr);
    if (status == -1) {
        destroyRWGMessage(msg);
        free(statusStr);
        printf("Error parsing enum\n");
        return NULL;
    }
    msg->status = status;
    free(statusStr);
    switch (msg->status) {
    case RWG_WIN:
    case RWG_DUP:
    case RWG_NOK:
    case RWG_OVR:
    case RWG_INV:
        if (sscanf(inBuffer, "RWG %*s %2u\n", &msg->trials) != 1) {
            destroyRWGMessage(msg);
            return NULL;
        }
        break;
    case RWG_ERR:
    default:
        if (strcmp(inBuffer, "RWG ERR\n") != 0) {
            destroyRWGMessage(msg);
            printf("Error comparins strings\n");
            return NULL;
        }
        break;
    }
    return msg;
}

void destroyQUTMessage(void *ptr) {
    QUTMessage *msg = (QUTMessage *)ptr;
    if (msg != NULL)
        free(msg->PLID);
    free(msg);
}

ssize_t serializeQUTMessage(void *ptr, char *outBuffer) {
    QUTMessage *msg = (QUTMessage *)ptr;
    return sprintf(outBuffer, "QUT %s\n", msg->PLID);
}

void *deserializeQUTMessage(char *inBuffer) {
    QUTMessage *msg = malloc(sizeof(QUTMessage));
    if (msg == NULL) {
        destroyQUTMessage(msg);
        return NULL;
    }
    msg->PLID = malloc(7 * sizeof(char));
    if (msg->PLID == NULL) {
        destroyQUTMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "QUT %6s\n", msg->PLID) != 1) {
        destroyQUTMessage(msg);
        return NULL;
    }
    return msg;
}

const char *RQTMessageStatusStrings[] = {"OK", "NOK", "ERR", NULL};

void destroyRQTMessage(void *ptr) {
    RQTMessage *msg = (RQTMessage *)ptr;
    free(msg);
}

ssize_t serializeRQTMessage(void *ptr, char *outBuffer) {
    RQTMessage *msg = (RQTMessage *)ptr;
    return sprintf(outBuffer, "RQT %s\n", RQTMessageStatusStrings[msg->status]);
}

void *deserializeRQTMessage(char *inBuffer) {
    RQTMessage *msg = malloc(sizeof(RQTMessage));
    char *statusStr = malloc(4 * sizeof(char));
    if (msg == NULL || statusStr == NULL) {
        destroyRQTMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "RQT %3s\n", statusStr) != 1) {
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

void destroyREVMessage(void *ptr) {
    REVMessage *msg = (REVMessage *)ptr;
    free(msg);
}

ssize_t serializeREVMessage(void *ptr, char *outBuffer) {
    REVMessage *msg = (REVMessage *)ptr;
    return sprintf(outBuffer, "REV %s\n", msg->PLID);
}

void *deserializeREVMessage(char *inBuffer) {
    REVMessage *msg = malloc(sizeof(REVMessage));
    if (msg == NULL) {
        destroyREVMessage(msg);
        return NULL;
    }
    msg->PLID = malloc(7 * sizeof(char));
    if (msg->PLID == NULL) {
        destroyREVMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "REV %6s\n", msg->PLID) != 1) {
        destroyREVMessage(msg);
        return NULL;
    }
    return msg;
}

const char *RRVMessageStatusStrings[] = {"OK", "ERR", NULL};

void destroyRRVMessage(void *ptr) {
    RRVMessage *msg = (RRVMessage *)ptr;
    if (msg != NULL)
        free(msg->word);
    free(msg);
}

ssize_t serializeRRVMessage(void *ptr, char *outBuffer) {
    RRVMessage *msg = (RRVMessage *)ptr;
    return sprintf(outBuffer, "RRV %s\n",
                   msg->type == RRV_STATUS
                       ? RRVMessageStatusStrings[msg->status]
                       : msg->word);
}

void *deserializeRRVMessage(char *inBuffer) {
    RRVMessage *msg = malloc(sizeof(RRVMessage));
    char *statusStr = malloc(4 * sizeof(char));
    if (msg == NULL || statusStr == NULL) {
        return NULL;
    }
    msg->word = malloc(31 * sizeof(char));
    if (msg->word == NULL) {
        return NULL;
    }
    if (sscanf(inBuffer, "RRV %3s\n", statusStr) != 1) {
        destroyRRVMessage(msg);
        return NULL;
    }
    int status = parseEnum(RRVMessageStatusStrings, statusStr);
    if (status != -1) {
        msg->type = RRV_STATUS;
        msg->word = NULL;
        msg->status = status;
    } else {
        msg->type = RRV_WORD;
        if (sscanf(inBuffer, "RRV %30s\n", msg->word) != 1) {
            destroyRRVMessage(msg);
            return NULL;
        }
    }
    return msg;
}

void destroyGSBMessage(void *ptr) { free(ptr); }

ssize_t serializeGSBMessage(UNUSED void *ptr, char *outBuffer) {
    return sprintf(outBuffer, "GSB\n");
}

void *deserializeGSBMessage(char *inBuffer) {
    GSBMessage *msg = malloc(sizeof(GSBMessage));
    if (msg == NULL || strcmp(inBuffer, "GSB\n") != 0) {
        destroyGSBMessage(msg);
        return NULL;
    }
    return msg;
}

void destroyGHLMessage(void *ptr) {
    GHLMessage *msg = (GHLMessage *)ptr;
    if (msg != NULL) {
        free(msg->PLID);
    }
    free(msg);
}

ssize_t serializeGHLMessage(void *ptr, char *outBuffer) {
    GHLMessage *msg = (GHLMessage *)ptr;
    return sprintf(outBuffer, "GHL %s\n", msg->PLID);
}

void *deserializeGHLMessage(char *inBuffer) {
    GHLMessage *msg = malloc(sizeof(GHLMessage));
    if (msg == NULL) {
        destroyGHLMessage(msg);
        return NULL;
    }
    msg->PLID = malloc(7 * sizeof(char));
    if (msg->PLID == NULL) {
        destroyGHLMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "GHL %6s\n", msg->PLID) != 1) {
        destroyGHLMessage(msg);
        return NULL;
    }
    return msg;
}

void destroySTAMessage(void *ptr) {
    STAMessage *msg = (STAMessage *)ptr;
    if (msg != NULL) {
        free(msg->PLID);
    }
    free(msg);
}

ssize_t serializeSTAMessage(void *ptr, char *outBuffer) {
    STAMessage *msg = (STAMessage *)ptr;
    return sprintf(outBuffer, "STA %s\n", msg->PLID);
}

void *deserializeSTAMessage(char *inBuffer) {
    STAMessage *msg = malloc(sizeof(STAMessage));
    if (msg == NULL) {
        destroySTAMessage(msg);
        return NULL;
    }
    msg->PLID = malloc(7 * sizeof(char));
    if (msg->PLID == NULL) {
        destroySTAMessage(msg);
        return NULL;
    }
    if (sscanf(inBuffer, "STA %s\n", msg->PLID) != 1) {
        destroySTAMessage(msg);
        return NULL;
    }
    return msg;
}

const char *RSBMessageStatusStrings[] = {"OK", "EMPTY", NULL};
const bool RSBMessageFileReceiveStatuses[] = {true, false, NULL};
const char *RHLMessageStatusStrings[] = {"OK", "NOK", NULL};
const bool RHLMessageFileReceiveStatuses[] = {true, false, NULL};
const char *RSTMessageStatusStrings[] = {"ACT", "FIN", "NOK", NULL};
const bool RSTMessageFileReceiveStatuses[] = {true, true, false, NULL};