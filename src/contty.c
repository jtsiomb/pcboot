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
	(((uint16_t)(c) & 0xff) | ((uint16_t)(attr) << 8))

static void scroll(void);
static void crtc_cursor(int x, int y);
static void crtc_setstart(int y);
static inline unsigned char crtc_read(int reg);
static inline void crtc_write(int reg, unsigned char val);

extern int cursor_x, cursor_y;
static unsigned char txattr = 0x07;
static int start_line;
static unsigned char cy0, cy1;
static int curvis;
static int scr_on = 1;

int con_init(void)
{
#ifdef CON_SERIAL
	ser_open(0, 9600, SER_8N1);
#endif

#ifdef CON_TEXTMODE
	cy0 = crtc_read(CRTC_REG_CURSTART);
	curvis = cy0 & 0x20 ? 1 : 0;
	cy0 &= 0x1f;
	cy1 = crtc_read(CRTC_REG_CUREND) & 0x1f;

	con_show_cursor(1);
	crtc_setstart(0);
	crtc_cursor(cursor_x, cursor_y);
	scr_on = 1;
#endif

	return 0;
}

void con_scr_enable(void)
{
	scr_on = 1;
}

void con_scr_disable(void)
{
	scr_on = 0;
}

void con_show_cursor(int show)
{
#ifdef CON_TEXTMODE
	unsigned char val = cy0 & 0x1f;
	if(!show) {
		val |= 0x20;
	}
	crtc_write(CRTC_REG_CURSTART, val);
	curvis = show;
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

void con_curattr(int shape, int blink)
{
#ifdef CON_TEXTMODE
	unsigned char start;
	cy0 = (shape == CON_CURSOR_LINE) ? 0xd : 0;
	cy1 = 0xe;

	start = cy0;
	if(curvis) {
		start |= 0x20;
	}

	crtc_write(CRTC_REG_CURSTART, start);
	crtc_write(CRTC_REG_CUREND, cy0);
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

void con_setattr(unsigned char attr)
{
	txattr = attr;
}

unsigned char con_getattr(void)
{
	return txattr;
}

void con_clear(void)
{
#ifdef CON_TEXTMODE
	memset16(TEXT_ADDR, VMEM_CHAR(' ', txattr), NCOLS * NROWS);

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
	if(scr_on) {
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

		case '\b':
			if(cursor_x > 0) cursor_x--;
			con_putchar_scr(cursor_x, cursor_y, ' ');
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
	}
#endif

#ifdef CON_SERIAL
	ser_putchar(c);
#endif
}

void con_putchar_scr(int x, int y, int c)
{
#ifdef CON_TEXTMODE
	uint16_t *ptr = (uint16_t*)TEXT_ADDR;
	ptr[(y + start_line) * NCOLS + x] = VMEM_CHAR(c, txattr);
#endif
}

int con_printf(int x, int y, const char *fmt, ...)
{
#ifdef CON_TEXTMODE
	va_list ap;
	char buf[81];
	char *ptr = buf;

	va_start(ap, fmt);
	vsnprintf(buf, 80, fmt, ap);
	va_end(ap);

	while(*ptr && x < 80) {
		con_putchar_scr(x++, y, *ptr++);
	}
	return ptr - buf;
#else
	return 0;
#endif
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
