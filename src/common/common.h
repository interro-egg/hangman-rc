#ifndef COMMON_H
#define COMMON_H

#define GN "043" // pad with zeros until 3 digits
#define GS_DEFAULT_PORT "58" GN

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#define UNUSED __attribute__((unused))

void exit_no_mem();
void lowercase(char *str);
int parseEnum(const char *strings[], char *to_parse);

#endif // COMMON_H