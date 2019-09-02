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
#ifndef STRING_H_
#define STRING_H_

#include <stdlib.h>

void *memset(void *s, int c, size_t n);
void *memset16(void *s, int c, size_t n);

void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);

int memcmp(void *aptr, void *bptr, size_t n);

size_t strlen(const char *s);

char *strchr(const char *s, int c);
char *strrchr(const char *s, int c);

char *strstr(const char *str, const char *substr);
char *strcasestr(const char *str, const char *substr);

int strcmp(const char *s1, const char *s2);
int strcasecmp(const char *s1, const char *s2);

int strncmp(const char *s1, const char *s2, int n);
int strncasecmp(const char *s1, const char *s2, int n);

char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);

char *strncpy(char *dest, const char *src, int n);

char *strerror(int err);

#endif	/* STRING_H_ */
