#ifndef COMMON_H
#define COMMON_H

#include "messages.h"

#define GN 1 // FIXME: use actual group number when assigned
#define GSPORT 58000 + GN

int *readUnsignedInt(char *inBuffer, unsigned int *into);

#endif // COMMON_H