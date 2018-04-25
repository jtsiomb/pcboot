#ifndef STDLIB_H_
#define STDLIB_H_

#include <inttypes.h>

typedef int32_t ssize_t;
typedef uint32_t size_t;

int atoi(const char *str);
long atol(const char *str);
long strtol(const char *str, char **endp, int base);

void itoa(int val, char *buf, int base);
void utoa(unsigned int val, char *buf, int base);

/* defined in malloc.c */
void *malloc(size_t sz);
void free(void *ptr);

#endif	/* STDLIB_H_ */
