/*
pcboot - bootable PC demo/game kernel
Copyright (C) 2018  John Tsiombikas <nuclear@member.fsf.org>

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

int putchar(int c);
int puts(const char *s);

int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);

int sprintf(char *buf, const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list ap);

int snprintf(char *buf, size_t sz, const char *fmt, ...);
int vsnprintf(char *buf, size_t sz, const char *fmt, va_list ap);

/* printf to the serial port */
int ser_printf(const char *fmt, ...);
int ser_vprintf(const char *fmt, va_list ap);

#endif	/* STDIO_H_ */
