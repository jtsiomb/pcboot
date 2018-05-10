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
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "contty.h"
#include "serial.h"
#include "asmops.h"
#include "config.h"

#define VIRT_ROWS	200

#define NCOLS		80
#define NROWS		25
#define TEXT_ADDR	((char*)0xb8000)

#define CRTC_ADDR	0x3d4
#define CRTC_DATA	0x3d5

#define CRTC_REG_CURSTART	0x0a
#define CRTC_REG_CUREND		0x0b
#define CRTC_REG_START_H	0x0c
#define CRTC_REG_START_L	0x0d
#define CRTC_REG_CURLOC_H	0x0e
#define CRTC_REG_CURLOC_L	0x0f

#define VMEM_CHAR(c, attr) \
	((uint16_t)(c) | ((uint16_t)(attr) << 8))

static void scroll(void);
static void crtc_cursor(int x, int y);
static void crtc_setstart(int y);
static inline unsigned char crtc_read(int reg);
static inline void crtc_write(int reg, unsigned char val);
static inline void crtc_write_bits(int reg, unsigned char val, unsigned char mask);

extern int cursor_x, cursor_y;
static unsigned char txattr = 0x07;
static int start_line;

int con_init(void)
{
#ifdef CON_SERIAL
	ser_open(0, 9600, SER_8N1);
#endif

#ifdef CON_TEXTMODE
	con_show_cursor(1);
	crtc_setstart(0);
	crtc_cursor(cursor_x, cursor_y);
	/*
	printf("curloc: %x %x\n", (unsigned int)crtc_read(CRTC_REG_CURLOC_H),
			(unsigned int)crtc_read(CRTC_REG_CURLOC_L));
	printf("curstart: %x\n", (unsigned int)crtc_read(CRTC_REG_CURSTART));
	printf("curend: %x\n", (unsigned int)crtc_read(CRTC_REG_CUREND));
	*/
#endif

	return 0;
}

void con_show_cursor(int show)
{
#ifdef CON_TEXTMODE
	unsigned char val = show ? 0 : 0x20;

	crtc_write_bits(CRTC_REG_CURSTART, val, 0x20);
#endif
}

void con_cursor(int x, int y)
{
#ifdef CON_TEXTMODE
	cursor_x = x;
	cursor_y = y;
	crtc_cursor(x, y);
#endif
}

void con_fgcolor(int c)
{
	txattr = (txattr & 0xf0) | c;
}

void con_bgcolor(int c)
{
	txattr = (txattr & 0x0f) | (c << 4);
}

void con_clear(void)
{
#ifdef CON_TEXTMODE
	memset(TEXT_ADDR, 0, NCOLS * NROWS * 2);

	start_line = 0;
	crtc_setstart(0);

	cursor_x = cursor_y = 0;
	crtc_cursor(0, 0);
#endif
}

static inline void linefeed(void)
{
	if(++cursor_y >= NROWS) {
		scroll();
		--cursor_y;
	}
}

void con_putchar(int c)
{
#ifdef CON_TEXTMODE
	uint16_t *ptr;

	switch(c) {
	case '\n':
		linefeed();
	case '\r':
		cursor_x = 0;
		crtc_cursor(cursor_x, cursor_y);
		break;

	case '\t':
		cursor_x = (cursor_x & 0x7) + 8;
		if(cursor_x >= NCOLS) {
			linefeed();
			cursor_x = 0;
		}
		crtc_cursor(cursor_x, cursor_y);
		break;

	default:
		con_putchar_scr(cursor_x, cursor_y, c);

		if(++cursor_x >= NCOLS) {
			linefeed();
			cursor_x = 0;
		}
		crtc_cursor(cursor_x, cursor_y);
	}
#endif

#ifdef CON_SERIAL
	ser_putchar(c);
#endif
}

void con_putchar_scr(int x, int y, int c)
{
	uint16_t *ptr = (uint16_t*)TEXT_ADDR;
	ptr[(y + start_line) * NCOLS + x] = VMEM_CHAR(c, txattr);
}

void con_printf(int x, int y, const char *fmt, ...)
{
	va_list ap;
	char buf[81];
	char *ptr = buf;

	va_start(ap, fmt);
	vsnprintf(buf, 80, fmt, ap);
	va_end(ap);

	while(*ptr && x < 80) {
		con_putchar_scr(x++, y, *ptr++);
	}
}

static void scroll(void)
{
	int new_line;

	if(++start_line > VIRT_ROWS - NROWS) {
		/* The bottom of the visible range reached the end of our text buffer.
		 * Copy the rest of the lines to the top and reset start_line.
		 */
		memcpy(TEXT_ADDR, TEXT_ADDR + start_line * NCOLS, (NROWS - 1) * NCOLS * 2);
		start_line = 0;
	}

	/* clear the next line that will be revealed by scrolling */
	new_line = start_line + NROWS - 1;
	memset16(TEXT_ADDR + new_line * NCOLS * 2, VMEM_CHAR(' ', txattr), NCOLS);
	crtc_setstart(start_line);
}

static void crtc_cursor(int x, int y)
{
	unsigned int addr;

	addr = (y + start_line) * NCOLS + x;

	crtc_write(CRTC_REG_CURLOC_L, addr);
	crtc_write(CRTC_REG_CURLOC_H, addr >> 8);
}

static void crtc_setstart(int y)
{
	unsigned int addr = y * NCOLS;

	crtc_write(CRTC_REG_START_L, addr);
	crtc_write(CRTC_REG_START_H, addr >> 8);
}

static inline unsigned char crtc_read(int reg)
{
	outb(reg, CRTC_ADDR);
	return inb(CRTC_DATA);
}

static inline void crtc_write(int reg, unsigned char val)
{
	outb(reg, CRTC_ADDR);
	outb(val, CRTC_DATA);
}

static inline void crtc_write_bits(int reg, unsigned char val, unsigned char mask)
{
	unsigned char prev;
	outb(reg, CRTC_ADDR);
	prev = inb(CRTC_DATA);
	val = (prev & ~mask) | (val & mask);
	outb(val, CRTC_DATA);
}
