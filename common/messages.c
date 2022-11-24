#include "messages.h"
#include <stdio.h>

char *serializeSNGMessage(SNGMessage *opts) {
  // "SNG " + PLID + "\n\0"
  int len = 4 + 6 + 2;
  char *msg = malloc(sizeof(char) * len);
  sprintf(msg, "SNG %6s\n", opts->PLID);
  free(opts->PLID);
  free(opts);
  return msg;
}

SNGMessage *deserializeSNGMessage(char *msg) {
  SNGMessage *opts = malloc(sizeof(SNGMessage));
  opts->PLID = malloc(sizeof(char) * 7);
  sscanf(msg, "SNG %6s", opts->PLID);
  free(msg);
  return opts;
}