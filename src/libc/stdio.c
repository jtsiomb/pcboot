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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "contty.h"
#include "serial.h"
#include "panic.h"

enum {
	OUT_DEF,
	OUT_BUF,
	OUT_SCR,
	OUT_SER
};

extern void pcboot_putchar(int c);

static int intern_printf(int out, char *buf, size_t sz, const char *fmt, va_list ap);
static int intern_scanf(const char *instr, FILE *infile, const char *fmt, va_list ap);
static void bwrite(int out, char *buf, size_t buf_sz, char *str, int sz);
/*static int readchar(const char *str, FILE *fp);*/

int putchar(int c)
{
	con_putchar(c);
	return c;
}

int puts(const char *s)
{
	while(*s) {
		putchar(*s++);
	}
	putchar('\n');
	return 0;
}

/* -- printf and friends -- */

int printf(const char *fmt, ...)
{
	int res;
	va_list ap;

	va_start(ap, fmt);
	res = intern_printf(OUT_DEF, 0, 0, fmt, ap);
	va_end(ap);
	return res;
}

int vprintf(const char *fmt, va_list ap)
{
	return intern_printf(OUT_DEF, 0, 0, fmt, ap);
}

int sprintf(char *buf, const char *fmt, ...)
{
	int res;
	va_list ap;

	va_start(ap, fmt);
	res = intern_printf(OUT_BUF, buf, 0, fmt, ap);
	va_end(ap);
	return res;
}

int vsprintf(char *buf, const char *fmt, va_list ap)
{
	return intern_printf(OUT_BUF, buf, 0, fmt, ap);
}

int snprintf(char *buf, size_t sz, const char *fmt, ...)
{
	int res;
	va_list ap;

	va_start(ap, fmt);
	res = intern_printf(OUT_BUF, buf, sz, fmt, ap);
	va_end(ap);
	return res;
}

int vsnprintf(char *buf, size_t sz, const char *fmt, va_list ap)
{
	return intern_printf(OUT_BUF, buf, sz, fmt, ap);
}

int fprintf(FILE *fp, const char *fmt, ...)
{
	int res;
	va_list ap;

	va_start(ap, fmt);
	res = vfprintf(fp, fmt, ap);
	va_end(ap);
	return res;
}

int vfprintf(FILE *fp, const char *fmt, va_list ap)
{
	if(fp == stdout || fp == stderr) {
		return vprintf(fmt, ap);
	}

	panic("*fprintf for anything other than stdout/stderr, not implemented yet\n");
	return 0;
}

int ser_printf(const char *fmt, ...)
{
	int res;
	va_list ap;

	va_start(ap, fmt);
	res = intern_printf(OUT_SER, 0, 0, fmt, ap);
	va_end(ap);
	return res;
}

int ser_vprintf(const char *fmt, va_list ap)
{
	return intern_printf(OUT_SER, 0, 0, fmt, ap);
}

void perror(const char *s)
{
	printf("%s: %s\n", s, strerror(errno));
}

/* intern_printf provides all the functionality needed by all the printf
 * variants.
 * - buf: optional buffer onto which the formatted results are written. If null
 *   then the output goes to the terminal through putchar calls. This is used
 *   by the (v)sprintf variants which write to an array of char.
 * - sz: optional maximum size of the output, 0 means unlimited. This is used
 *   by the (v)snprintf variants to avoid buffer overflows.
 * The rest are obvious, format string and variable argument list.
 */
static char *convc = "dioxXucsfeEgGpn%";

#define IS_CONV(c)	strchr(convc, c)

#define BUF(x)	((x) ? (x) + cnum : (x))
#define SZ(x)	((x) ? (x) - cnum : (x))

static int intern_printf(int out, char *buf, size_t sz, const char *fmt, va_list ap)
{
	char conv_buf[32];
	char *str;
	int i, slen;
	const char *fstart = 0;

	/* state */
	int cnum = 0;
	int base = 10;
	int alt = 0;
	int fwidth = 0;
	int padc = ' ';
	int sign = 0;
	int left_align = 0;
	int hex_caps = 0;
	int unsig = 0;
	int num, unum;

	while(*fmt) {
		if(*fmt == '%') {
			fstart = fmt++;
			continue;
		}

		if(fstart) {
			if(IS_CONV(*fmt)) {
				switch(*fmt) {
				case 'X':
					hex_caps = 1;

				case 'x':
				case 'p':
					base = 16;

					if(alt) {
						bwrite(out, BUF(buf), SZ(sz), "0x", 2);
						cnum += 2;
					}

				case 'u':
					unsig = 1;

					if(0) {
				case 'o':
						base = 8;

						if(alt) {
							bwrite(out, BUF(buf), SZ(sz), "0", 1);
							cnum++;
						}
					}

				case 'd':
				case 'i':
					if(unsig) {
						unum = va_arg(ap, unsigned int);
						utoa(unum, conv_buf, base);
					} else {
						num = va_arg(ap, int);
						itoa(num, conv_buf, base);
					}
					if(hex_caps) {
						for(i=0; conv_buf[i]; i++) {
							conv_buf[i] = toupper(conv_buf[i]);
						}
					}

					slen = strlen(conv_buf);

					if(left_align) {
						if(!unsig && sign && num >= 0) {
							bwrite(out, BUF(buf), SZ(sz), "+", 1);
							cnum++;
						}
						bwrite(out, BUF(buf), SZ(sz), conv_buf, slen);
						cnum += slen;
						padc = ' ';
					}
					for(i=slen; i<fwidth; i++) {
						bwrite(out, BUF(buf), SZ(sz), (char*)&padc, 1);
						cnum++;
					}
					if(!left_align) {
						if(!unsig && sign && num >= 0) {
							bwrite(out, BUF(buf), SZ(sz), "+", 1);
							cnum++;
						}
						bwrite(out, BUF(buf), SZ(sz), conv_buf, slen);
						cnum += slen;
					}
					break;

				case 'c':
					{
						char c = va_arg(ap, int);
						bwrite(out, BUF(buf), SZ(sz), &c, 1);
						cnum++;
					}
					break;

				case 's':
					str = va_arg(ap, char*);
					slen = strlen(str);

					if(left_align) {
						bwrite(out, BUF(buf), SZ(sz), str, slen);
						cnum += slen;
						padc = ' ';
					}
					for(i=slen; i<fwidth; i++) {
						bwrite(out, BUF(buf), SZ(sz), (char*)&padc, 1);
						cnum++;
					}
					if(!left_align) {
						bwrite(out, BUF(buf), SZ(sz), str, slen);
						cnum += slen;
					}
					break;

				case 'n':
					*va_arg(ap, int*) = cnum;
					break;

				default:
					break;
				}

				/* restore default conversion state */
				base = 10;
				alt = 0;
				fwidth = 0;
				padc = ' ';
				hex_caps = 0;

				fstart = 0;
				fmt++;
			} else {
				switch(*fmt) {
				case '#':
					alt = 1;
					break;

				case '+':
					sign = 1;
					break;

				case '-':
					left_align = 1;
					break;

				case 'l':
				case 'L':
					break;

				case '0':
					padc = '0';
					break;

				default:
					if(isdigit(*fmt)) {
						const char *fw = fmt;
						while(*fmt && isdigit(*fmt)) fmt++;

						fwidth = atoi(fw);
						continue;
					}
				}
				fmt++;
			}
		} else {
			bwrite(out, BUF(buf), SZ(sz), (char*)fmt++, 1);
			cnum++;
		}
	}

	return cnum;
}


#if 0
static char *sconvc = "diouxcsefg%";

#define IS_SCONV(c)	strchr(sconvc, c)

static int intern_scanf(const char *instr, FILE *infile, const char *fmt, va_list ap)
{
	return -1;	/* TODO */
}
#endif


/* bwrite is called by intern_printf to transparently handle writing into a
 * buffer or to the terminal
 */
static void bwrite(int out, char *buf, size_t buf_sz, char *str, int sz)
{
	int i;

	if(out == OUT_BUF) {
		if(buf_sz && buf_sz <= sz) sz = buf_sz;
		buf[sz] = 0;
		memcpy(buf, str, sz);
	} else {
		switch(out) {
		case OUT_DEF:
			for(i=0; i<sz; i++) {
				putchar(*str++);
			}
			break;

		case OUT_SER:
			for(i=0; i<sz; i++) {
				ser_putchar(*str++);
			}
			break;

		default:
			/* TODO: OUT_SCR */
			break;
		}
	}
}

/*
static int readchar(const char *str, FILE *fp)
{
	static const char *orig_str;
	static const char *sptr;

	if(str) {
		if(str == orig_str) {
			if(!*sptr) return -1;
			return *sptr++;
		} else {
			orig_str = sptr = str;
			return readchar(str, fp);
		}
	} else {
		return fgetc(fp);
	}

	return -1;
}
*/
