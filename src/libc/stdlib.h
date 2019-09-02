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
#ifndef STDLIB_H_
#define STDLIB_H_

#include <stddef.h>

#define RAND_MAX	2147483647

#define abs(x)	__builtin_abs(x)

int atoi(const char *str);
long atol(const char *str);
long strtol(const char *str, char **endp, int base);

void itoa(int val, char *buf, int base);
void utoa(unsigned int val, char *buf, int base);

double atof(const char *str);
double strtod(const char *str, char **endp);

int atexit(void (*func)(void));

void abort(void);

void qsort(void *arr, size_t count, size_t size, int (*cmp)(const void*, const void*));

int rand(void);
int rand_r(unsigned int *seedp);
void srand(unsigned int seed);

/* defined in malloc.c */
void *malloc(size_t sz);
void *calloc(size_t num, size_t sz);
void *realloc(void *ptr, size_t sz);
void free(void *ptr);

#endif	/* STDLIB_H_ */
