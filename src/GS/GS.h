#ifndef GS_H
#define GS_H

#include "../common/common.h"
#include "server_state.h"

#define USAGE_FMT "Usage: %s word_file [-p GSport] [-v]\n"

void readOpts(int argc, char *argv[], char **word_file, char **port,
              bool *verbose);

#endif // GS_H