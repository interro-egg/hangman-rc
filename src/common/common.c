#include "common.h"
#include <string.h>

int parseEnum(const char *strings[], char *to_parse) {
  for (unsigned int i = 0; i < sizeof(&strings) / sizeof(char *); i++) {
    if (strcmp(strings[i], to_parse) == 0) {
      return (int)i;
    }
  };
  return -1;
}