#include "common.h"

int parse_enum(char *strings[], char *to_parse) {
    for (int i = 0; i < sizeof(strings) / sizeof(char *); i++) {
        if (strcmp(strings[i], to_parse) == 0) {
            return i;
        }
    };
    return -1;
}