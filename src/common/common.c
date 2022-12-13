#include "common.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void exit_no_mem() {
    perror("Out of memory!\n");
    exit(EXIT_FAILURE);
}

void lowercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = (char)tolower(str[i]);
    }
}

int parseEnum(const char *strings[], char *to_parse) {
    for (unsigned int i = 0; i < sizeof(&strings) / sizeof(char *); i++) {
        if (strcmp(strings[i], to_parse) == 0) {
            return (int)i;
        }
    };
    return -1;
}