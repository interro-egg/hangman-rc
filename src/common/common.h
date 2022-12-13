#ifndef COMMON_H
#define COMMON_H

#define GN "043" // pad with zeros until 3 digits
#define GS_DEFAULT_PORT "58" GN

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define UNUSED __attribute__((unused))

void exitNoMem();
void lowercase(char *str);
int parseEnum(const char *strings[], char *toParse);

#endif // COMMON_H