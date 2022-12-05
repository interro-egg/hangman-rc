#ifndef MESSAGES_H
#define MESSAGES_H

// Start New Game

typedef struct SNGMessage {
  char *PLID;
} SNGMessage;

ssize_t serializeSNGMessage(SNGMessage *msg, char* outBuffer);
SNGMessage *deserializeSNGMessage(char *inBuffer);

// Response: Start Game

typedef struct RSGMessage {
  enum RSGMessageStatus status;
  unsigned int n_letters;
  unsigned int max_errors;
} RSGMessage;

enum RSGMessageStatus { OK, NOK };

ssize_t serializeRSGMessage(RSGMessage *msg, char* outBuffer);
RSGMessage *deserializeRSGMessage(char *inBuffer);

// Letter Guess

typedef struct PLGMessage {
  char *PLID;
  char letter;
  unsigned int trial;
} PLGMessage;

ssize_t serializePLGMessage(PLGMessage *msg, char* outBuffer);
PLGMessage *deserializePLGMessage(char *inBuffer);

// Response: Letter Guess

typedef struct RLGMessage {
  enum RLGMessageStatus status;
  unsigned int trial;
  unsigned int n;
  unsigned int *pos;
} RLGMessage;

enum RLGMessageStatus { OK, WIN, DUP, NOK, OVR, INV, ERR };

ssize_t serializeRLGMessage(RLGMessage *msg, char* outBuffer);
RLGMessage *deserializeRLGMessage(char *inBuffer);

// Word Guess

typedef struct PWGMessage {
  char *PLID;
  char *word;
  unsigned int trial;
} PWGMessage;

ssize_t serializePWGMessage(PWGMessage *msg, char* outBuffer);
PWGMessage *deserializePWGMessage(char *inBuffer);

// Response: Word Guess

typedef struct RWGMessage {
  enum RWGMessageStatus status;
  unsigned int trials;
} RWGMessage;

enum RWGMessageStatus { WIN, NOK, OVR, INV, ERR };

ssize_t serializeRWGMessage(RWGMessage *msg, char* outBuffer);
RWGMessage *deserializeRWGMessage(char *inBuffer);

// Quit

typedef struct QUTMessage {
  char *PLID;
} QUTMessage;

ssize_t serializeQUTMessage(QUTMessage *msg, char* outBuffer);
QUTMessage *deserializeQUTMessage(char *inBuffer);

// Response: Quit

typedef struct RQTMessage {
  enum RQTMessageStatus status;
} RQTMessage;

enum RQTMessageStatus { OK, ERR };

ssize_t serializeRQTMessage(RQTMessage *msg, char* outBuffer);
RQTMessage *deserializeRQTMessage(char *inBuffer);

// Reveal

typedef struct REVMessage {
  char *PLID;
} REVMessage;

ssize_t serializeREVMessage(REVMessage *msg, char* outBuffer);
REVMessage *deserializeREVMessage(char *inBuffer);

// Response: Reveal

typedef struct RRVMessage {
  enum RRVMessageType type;
  char *word;
  enum RRVMessageStatus status;
} RRVMessage;

enum RRVMessageType { WORD, STATUS };

enum RRVMessageStatus { OK, ERR };

ssize_t serializeRRVMessage(RRVMessage *msg, char* outBuffer);
RRVMessage *deserializeRRVMessage(char *inBuffer);

#endif // MESSAGES_H