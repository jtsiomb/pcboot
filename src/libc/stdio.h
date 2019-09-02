/*
pcboot - bootable PC demo/game kernel
Copyright (C) 2018-2019  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY, without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef STDIO_H_
#define STDIO_H_

#include <stdlib.h>
#include <stdarg.h>

typedef struct FILE FILE;

#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2

#define EOF	(-1)

#define stdin	((FILE*)0)
#define stdout	((FILE*)1)
#define stderr	((FILE*)2)

int putchar(int c);
int puts(const char *s);

int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);

int sprintf(char *buf, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list ap);

int snprintf(char *buf, size_t sz, const char *fmt, ...);
int vsnprintf(char *buf, size_t sz, const char *fmt, va_list ap);

/* TODO */
int fprintf(FILE *fp, const char *fmt, ...);
int vfprintf(FILE *fp, const char *fmt, va_list ap);

/* TODO
int fscanf(FILE *fp, const char *fmt, ...);
int vfscanf(FILE *fp, const char *fmt, va_list ap);

int sscanf(const char *str, const char *fmt, ...);
int vsscanf(const char *ptr, const char *fmt, va_list ap);
*/

/* printf to the serial port */
int ser_printf(const char *fmt, ...);
int ser_vprintf(const char *fmt, va_list ap);

void perror(const char *s);


/* FILE I/O */
FILE *fopen(const char *path, const char *mode);
int fclose(FILE *fp);

long filesize(FILE *fp);
int fseek(FILE *fp, long offset, int from);
void rewind(FILE *fp);
long ftell(FILE *fp);

size_t fread(void *buf, size_t size, size_t count, FILE *fp);
size_t fwrite(const void *buf, size_t size, size_t count, FILE *fp);

int fgetc(FILE *fp);
char *fgets(char *buf, int size, FILE *fp);

int fputc(int c, FILE *fp);

int fflush(FILE *fp);

int feof(FILE *fp);
int ferror(FILE *fp);
void clearerr(FILE *fp);

#endif	/* STDIO_H_ */
