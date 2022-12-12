#ifndef COMMON_H
#define COMMON_H

#include "messages.h"
#include <string.h>

#define GN 1 // FIXME: use actual group number when assigned
#define GSPORT 58000 + GN

int *readUnsignedInt(char *inBuffer, unsigned int *into);
int parse_enum(char *strings[], char *to_parse);

#endif // COMMON_H