#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdio.h>

// Start New Game

typedef struct SNGMessage {
    char *PLID;
} SNGMessage;

void destroySNGMessage(SNGMessage *msg);
ssize_t serializeSNGMessage(void *ptr, char *outBuffer);
void *deserializeSNGMessage(char *inBuffer);

// Response: Start Game

enum RSGMessageStatus { RSG_OK, RSG_NOK };
extern const char *RSGMessageStatusStrings[];

typedef struct RSGMessage {
    enum RSGMessageStatus status;
    unsigned int n_letters;
    unsigned int max_errors;
} RSGMessage;

void destroyRSGMessage(RSGMessage *msg);
ssize_t serializeRSGMessage(void *ptr, char *outBuffer);
void *deserializeRSGMessage(char *inBuffer);

// Letter Guess

typedef struct PLGMessage {
    char *PLID;
    char letter;
    unsigned int trial;
} PLGMessage;

void destroyPLGMessage(PLGMessage *msg);
ssize_t serializePLGMessage(void *ptr, char *outBuffer);
void *deserializePLGMessage(char *inBuffer);

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
extern const char *RLGMessageStatusStrings[];

typedef struct RLGMessage {
    enum RLGMessageStatus status;
    unsigned int trial;
    unsigned int n;
    unsigned int *pos;
} RLGMessage;

void destroyRLGMessage(RLGMessage *msg);
ssize_t serializeRLGMessage(void *ptr, char *outBuffer);
void *deserializeRLGMessage(char *inBuffer);

// Word Guess

typedef struct PWGMessage {
    char *PLID;
    char *word;
    unsigned int trial;
} PWGMessage;

void destroyPWGMessage(PWGMessage *msg);
ssize_t serializePWGMessage(void *ptr, char *outBuffer);
void *deserializePWGMessage(char *inBuffer);

// Response: Word Guess

enum RWGMessageStatus { RWG_WIN, RWG_DUP, RWG_NOK, RWG_OVR, RWG_INV, RWG_ERR };
extern const char *RWGstaticMessageStatusStrings[];

typedef struct RWGMessage {
    enum RWGMessageStatus status;
    unsigned int trials;
} RWGMessage;

void destroyRWGMessage(RWGMessage *msg);
ssize_t serializeRWGMessage(void *ptr, char *outBuffer);
void *deserializeRWGMessage(char *inBuffer);

// Quit

typedef struct QUTMessage {
    char *PLID;
} QUTMessage;

void destroyQUTMessage(QUTMessage *msg);
ssize_t serializeQUTMessage(void *ptr, char *outBuffer);
void *deserializeQUTMessage(char *inBuffer);

// Response: Quit

enum RQTMessageStatus { RQT_OK, RQT_ERR };
extern const char *RQTMessageStatusStrings[];

typedef struct RQTMessage {
    enum RQTMessageStatus status;
} RQTMessage;

void destroyRQTMessage(RQTMessage *msg);
ssize_t serializeRQTMessage(void *ptr, char *outBuffer);
void *deserializeRQTMessage(char *inBuffer);

// Reveal

typedef struct REVMessage {
    char *PLID;
} REVMessage;

void destroyREVMessage(REVMessage *msg);
ssize_t serializeREVMessage(void *ptr, char *outBuffer);
void *deserializeREVMessage(char *inBuffer);

// Response: Reveal

enum RRVMessageType { RRV_WORD, RRV_STATUS };
enum RRVMessageStatus { RRV_OK, RRV_ERR };
extern const char *RRVMessageStatusStrings[];

typedef struct RRVMessage {
    enum RRVMessageType type;
    char *word;
    enum RRVMessageStatus status;
} RRVMessage;

void destroyRRVMessage(RRVMessage *msg);
ssize_t serializeRRVMessage(void *ptr, char *outBuffer);
void *deserializeRRVMessage(char *inBuffer);

#endif // MESSAGES_H