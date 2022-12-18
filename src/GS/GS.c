#include "GS.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

ServerState serverState = {NULL, GS_DEFAULT_PORT, false};

int main(int argc, char *argv[]) {
    readOpts(argc, argv, &(serverState.word_file), &(serverState.port),
             &(serverState.verbose));

    if (serverState.word_file == NULL) {
        fprintf(stderr, USAGE_FMT, argv[0]);
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}

void readOpts(int argc, char *argv[], char **word_file, char **port,
              bool *verbose) {
    int opt;

    while ((opt = getopt(argc, argv, "-p:v")) != -1) {
        switch (opt) {
        case 1:
            if (*word_file != NULL) {
                fprintf(stderr, USAGE_FMT, argv[0]);
                exit(EXIT_FAILURE);
            }
            *word_file = optarg;
            break;
        case 'p':
            *port = optarg;
            break;
        case 'v':
            *verbose = true;
            break;
        default:
            fprintf(stderr, USAGE_FMT, argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}