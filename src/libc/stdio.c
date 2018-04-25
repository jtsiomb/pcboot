#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "contty.h"

extern void pcboot_putchar(int c);

static void bwrite(char *buf, size_t buf_sz, char *str, int sz);
static int intern_printf(char *buf, size_t sz, const char *fmt, va_list ap);

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

static char *convc = "dioxXucsfeEgGpn%";

#define IS_CONV(c)	strchr(convc, c)

int printf(const char *fmt, ...)
{
	int res;
	va_list ap;

	va_start(ap, fmt);
	res = intern_printf(0, 0, fmt, ap);
	va_end(ap);
	return res;
}

int vprintf(const char *fmt, va_list ap)
{
	return intern_printf(0, 0, fmt, ap);
}

int sprintf(char *buf, const char *fmt, ...)
{
	int res;
	va_list ap;

	va_start(ap, fmt);
	res = intern_printf(buf, 0, fmt, ap);
	va_end(ap);
	return res;
}

int vsprintf(char *buf, const char *fmt, va_list ap)
{
	return intern_printf(buf, 0, fmt, ap);
}

int snprintf(char *buf, size_t sz, const char *fmt, ...)
{
	int res;
	va_list ap;

	va_start(ap, fmt);
	res = intern_printf(buf, sz, fmt, ap);
	va_end(ap);
	return res;
}

int vsnprintf(char *buf, size_t sz, const char *fmt, va_list ap)
{
	return intern_printf(buf, sz, fmt, ap);
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

#define BUF(x)	((x) ? (x) + cnum : (x))
#define SZ(x)	((x) ? (x) - cnum : (x))

static int intern_printf(char *buf, size_t sz, const char *fmt, va_list ap)
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
	int left_align = 0;	/* not implemented yet */
	int hex_caps = 0;
	int unsig = 0;

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
						bwrite(BUF(buf), SZ(sz), "0x", 2);
					}

				case 'u':
					unsig = 1;

					if(0) {
				case 'o':
						base = 8;

						if(alt) {
							bwrite(BUF(buf), SZ(sz), "0", 1);
						}
					}

				case 'd':
				case 'i':
					if(unsig) {
						utoa(va_arg(ap, unsigned int), conv_buf, base);
					} else {
						itoa(va_arg(ap, int), conv_buf, base);
					}
					if(hex_caps) {
						for(i=0; conv_buf[i]; i++) {
							conv_buf[i] = toupper(conv_buf[i]);
						}
					}

					slen = strlen(conv_buf);
					for(i=slen; i<fwidth; i++) {
						bwrite(BUF(buf), SZ(sz), (char*)&padc, 1);
						cnum++;
					}

					bwrite(BUF(buf), SZ(sz), conv_buf, strlen(conv_buf));
					cnum += slen;
					break;

				case 'c':
					{
						char c = va_arg(ap, int);
						bwrite(BUF(buf), SZ(sz), &c, 1);
						cnum++;
					}
					break;

				case 's':
					str = va_arg(ap, char*);
					slen = strlen(str);

					for(i=slen; i<fwidth; i++) {
						bwrite(BUF(buf), SZ(sz), (char*)&padc, 1);
						cnum++;
					}
					bwrite(BUF(buf), SZ(sz), str, slen);
					cnum += slen;
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
			bwrite(BUF(buf), SZ(sz), (char*)fmt++, 1);
			cnum++;
		}
	}

	return 0;
}


/* bwrite is called by intern_printf to transparently handle writing into a
 * buffer (if buf is non-null) or to the terminal (if buf is null).
 */
static void bwrite(char *buf, size_t buf_sz, char *str, int sz)
{
	if(buf) {
		if(buf_sz && buf_sz <= sz) sz = buf_sz - 1;
		memcpy(buf, str, sz);

		buf[sz] = 0;
	} else {
		int i;
		for(i=0; i<sz; i++) {
			putchar(*str++);
		}
	}
}

