#include "GS.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s word_file [-p GSport] [-v]\n", argv[0]);
    }

    char *port = GS_DEFAULT_PORT;
    bool verbose = 0;

    readOpts(argc, argv, &port, &verbose);

    return EXIT_SUCCESS;
}

void readOpts(int argc, char *argv[], char **port, bool *verbose) {
    int opt;

    while ((opt = getopt(argc, argv, "p:v")) != -1) {
        switch (opt) {
        case 'p':
            *port = optarg;
            break;
        case 'v':
            *verbose = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-n GSIP] [-p GSport]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}