#ifndef STDIO_H_
#define STDIO_H_

#include <stdlib.h>
#include <stdarg.h>

int putchar(int c);
int puts(const char *s);

int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);

int sprintf(char *buf, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list ap);

int snprintf(char *buf, size_t sz, const char *fmt, ...);
int vsnprintf(char *buf, size_t sz, const char *fmt, va_list ap);

#endif	/* STDIO_H_ */
