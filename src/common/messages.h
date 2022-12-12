#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdio.h>

// Start New Game

typedef struct SNGMessage {
    char *PLID;
} SNGMessage;

ssize_t serializeSNGMessage(SNGMessage *msg, char *outBuffer);
SNGMessage *deserializeSNGMessage(char *inBuffer);

// Response: Start Game

enum RSGMessageStatus { RSG_OK, RSG_NOK };
static const char *RSGMessageStatusStrings[] = {"OK", "NOK"};

typedef struct RSGMessage {
    enum RSGMessageStatus status;
    unsigned int n_letters;
    unsigned int max_errors;
} RSGMessage;

ssize_t serializeRSGMessage(RSGMessage *msg, char *outBuffer);
RSGMessage *deserializeRSGMessage(char *inBuffer);

// Letter Guess

typedef struct PLGMessage {
    char *PLID;
    char letter;
    unsigned int trial;
} PLGMessage;

ssize_t serializePLGMessage(PLGMessage *msg, char *outBuffer);
PLGMessage *deserializePLGMessage(char *inBuffer);

// Response: Letter Guess

enum RLGMessageStatus {
    RLG_OK,
    RLG_WIN,
    RLG_DUP,
    RLG_NOK,
    RLG_OVR,
    RLG_INV,
    RLG_ERR
};
static const char *RLGMessageStatusStrings[] = {"OK",  "WIN", "DUP", "NOK",
                                                "OVR", "INV", "ERR"};

typedef struct RLGMessage {
    enum RLGMessageStatus status;
    unsigned int trial;
    unsigned int n;
    unsigned int *pos;
} RLGMessage;

ssize_t serializeRLGMessage(RLGMessage *msg, char *outBuffer);
RLGMessage *deserializeRLGMessage(char *inBuffer);

// Word Guess

typedef struct PWGMessage {
    char *PLID;
    char *word;
    unsigned int trial;
} PWGMessage;

ssize_t serializePWGMessage(PWGMessage *msg, char *outBuffer);
PWGMessage *deserializePWGMessage(char *inBuffer);

// Response: Word Guess

enum RWGMessageStatus { RWG_WIN, RWG_NOK, RWG_OVR, RWG_INV, RWG_ERR };
static const char *RWGMessageStatusStrings[] = {"WIN", "NOK", "OVR", "INV",
                                                "ERR"};

typedef struct RWGMessage {
    enum RWGMessageStatus status;
    unsigned int trials;
} RWGMessage;

ssize_t serializeRWGMessage(RWGMessage *msg, char *outBuffer);
RWGMessage *deserializeRWGMessage(char *inBuffer);

// Quit

typedef struct QUTMessage {
    char *PLID;
} QUTMessage;

ssize_t serializeQUTMessage(QUTMessage *msg, char *outBuffer);
QUTMessage *deserializeQUTMessage(char *inBuffer);

// Response: Quit

enum RQTMessageStatus { RQT_OK, RQT_ERR };
static const char *RQTMessageStatusStrings[] = {"OK", "ERR"};

typedef struct RQTMessage {
    enum RQTMessageStatus status;
} RQTMessage;

ssize_t serializeRQTMessage(RQTMessage *msg, char *outBuffer);
RQTMessage *deserializeRQTMessage(char *inBuffer);

// Reveal

typedef struct REVMessage {
    char *PLID;
} REVMessage;

ssize_t serializeREVMessage(REVMessage *msg, char *outBuffer);
REVMessage *deserializeREVMessage(char *inBuffer);

// Response: Reveal

enum RRVMessageType { RRV_WORD, RRV_STATUS };
static const char *RRVMessageTypeStrings[] = {"WORD", "STATUS"};
enum RRVMessageStatus { RRV_OK, RRV_ERR };
static const char *RRVMessageStatusStrings[] = {"OK", "ERR"};

typedef struct RRVMessage {
    enum RRVMessageType type;
    char *word;
    enum RRVMessageStatus status;
} RRVMessage;

ssize_t serializeRRVMessage(RRVMessage *msg, char *outBuffer);
RRVMessage *deserializeRRVMessage(char *inBuffer);

#endif // MESSAGES_H