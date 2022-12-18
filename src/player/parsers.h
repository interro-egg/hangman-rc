#ifndef PARSERS_H
#define PARSERS_H

#include "../common/messages.h"

void *parseSNGArgs(char *args);
void *parsePLGArgs(char *args);
void *parsePWGArgs(char *args);
void *parseQUTArgs(char *args);
void *parseREVArgs(char *args);

void *parseGSBArgs(char *args);
void *parseGHLArgs(char *args);
void *parseSTAArgs(char *args);

#endif // PARSERS_H