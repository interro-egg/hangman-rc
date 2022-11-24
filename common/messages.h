#ifndef MESSAGES_H
#define MESSAGES_H

// Start New Game

typedef struct SNGMessage {
  char *PLID;
} SNGMessage;

char *serializeSNGMessage(SNGMessage *opts);
SNGMessage *deserializeSNGMessage(char *msg);

// Response: Start Game

typedef struct RSGMessage {
  enum RSGMessageStatus status;
  unsigned int n_letters;
  unsigned int max_errors;
} RSGMessage;

enum RSGMessageStatus { OK, NOK };

char *serializeRSGMessage(RSGMessage *opts);
RSGMessage *deserializeRSGMessage(char *msg);

// Letter Guess

typedef struct PLGMessage {
  char *PLID;
  char letter;
  unsigned int trial;
} PLGMessage;

char *serializePLGMessage(PLGMessage *opts);
PLGMessage *deserializePLGMessage(char *msg);

// Response: Letter Guess

typedef struct RLGMessage {
  enum RLGMessageStatus status;
  unsigned int trial;
  unsigned int n;
  unsigned int *pos;
} RLGMessage;

enum RLGMessageStatus { OK, WIN, DUP, NOK, OVR, INV, ERR };

char *serializeRLGMessage(RLGMessage *opts);
RLGMessage *deserializeRLGMessage(char *msg);

// Word Guess

typedef struct PWGMessage {
  char *PLID;
  char *word;
  unsigned int trial;
} PWGMessage;

char *serializePWGMessage(PWGMessage *opts);
PWGMessage *deserializePWGMessage(char *msg);

// Response: Word Guess

typedef struct RWGMessage {
  enum RWGMessageStatus status;
  unsigned int trials;
} RWGMessage;

enum RWGMessageStatus { WIN, NOK, OVR, INV, ERR };

char *serializeRWGMessage(RWGMessage *opts);
RWGMessage *deserializeRWGMessage(char *msg);

// Quit

typedef struct QUTMessage {
  char *PLID;
} QUTMessage;

char *serializeQUTMessage(QUTMessage *opts);
QUTMessage *deserializeQUTMessage(char *msg);

// Response: Quit

typedef struct RQTMessage {
  enum RQTMessageStatus status;
} RQTMessage;

enum RQTMessageStatus { OK, ERR };

char *serializeRQTMessage(RQTMessage *opts);
RQTMessage *deserializeRQTMessage(char *msg);

// Reveal

typedef struct REVMessage {
  char *PLID;
} REVMessage;

char *serializeREVMessage(REVMessage *opts);
REVMessage *deserializeREVMessage(char *msg);

// Response: Reveal

typedef struct RRVMessage {
  enum RRVMessageType type;
  char *word;
  enum RRVMessageStatus status;
} RRVMessage;

enum RRVMessageType { WORD, STATUS };

enum RRVMessageStatus { OK, ERR };

char *serializeRRVMessage(RRVMessage *opts);
RRVMessage *deserializeRRVMessage(char *msg);

#endif // MESSAGES_H