#ifndef PARSERS_H
#define PARSERS_H

#include "../common/messages.h"

SNGMessage *parseSNGArgs(char *args);
PLGMessage *parsePLGArgs(char *args);
PWGMessage *parsePWGArgs(char *args);


#endif // PARSERS_H