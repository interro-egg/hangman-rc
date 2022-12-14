#ifndef COMMON_H
#define COMMON_H

#include <netdb.h>
#include <stdbool.h>

#define GN "043" // pad with zeros until 3 digits
#define GS_DEFAULT_PORT "58" GN

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define MAX_WORD_SIZE 30
#define MIN_WORD_SIZE 3
#define PLID_SIZE 6

#define MAX_FNAME_LEN 24
#define MAX_FSIZE_LEN 10
#define MAX_FSIZE_NUM 0x40000000 // 1 GB

#define UNUSED __attribute__((unused))

#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

void lowercase(char *str);
int parseEnum(const char *strings[], char *toParse);
int getAddrInfoSockType(char *host, char *port, struct addrinfo **addr,
                        int sockType, bool passive);
int checkPLID(char *PLID);

#endif // COMMON_H