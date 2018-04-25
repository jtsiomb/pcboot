#ifndef STRING_H_
#define STRING_H_

#include <stdlib.h>

void memset(void *s, int c, size_t n);
void memset16(void *s, int c, size_t n);

void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);

size_t strlen(const char *s);

char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);

char *strstr(const char *str, const char *substr);

int strcmp(const char *s1, const char *s2);

#endif	/* STRING_H_ */
