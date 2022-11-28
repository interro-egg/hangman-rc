#include "util.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

void exit_no_mem() {
    fprintf(stderr, "Out of memory!\n");
    exit(EXIT_FAILURE);
}

void lowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = (char)tolower(str[i]);
    }
}