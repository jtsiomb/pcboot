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
#ifndef CONTTY_H_
#define CONTTY_H_

enum {
	CON_CURSOR_LINE,
	CON_CURSOR_BLOCK
};

enum {
	BLACK,
	BLUE,
	GREEN,
	CYAN,
	RED,
	MAGENTA,
	BROWN,
	LTGREY,
	GREY,
	LTBLUE,
	LTGREEN,
	LTCYAN,
	LTRED,
	LTMAGENTA,
	YELLOW,
	WHITE
};
#define BRIGHT		8
#define FG_BRIGHT	0x08
#define BG_BRIGHT	0x80

enum {
	G_DIAMOND	= 0x04,
	G_CHECKER	= 0xb1,
	G_LR_CORNER	= 0xd9,
	G_UR_CORNER	= 0xbf,
	G_UL_CORNER	= 0xda,
	G_LL_CORNER	= 0xc0,
	G_CROSS		= 0xc5,
	G_HLINE		= 0xc4,
	G_L_TEE		= 0xc3,
	G_R_TEE		= 0xb4,
	G_B_TEE		= 0xc1,
	G_T_TEE		= 0xc2,
	G_VLINE		= 0xb3,
	G_CDOT		= 0xf8,

	G_HDBL		= 0xcd,
	G_UL_HDBL	= 0xd5,
	G_UR_HDBL	= 0xb8,
	G_T_HDBL_TEE = 0xd1
};


int con_init(void);
void con_scr_enable(void);
void con_scr_disable(void);
void con_show_cursor(int show);
void con_cursor(int x, int y);
void con_curattr(int shape, int blink);
void con_fgcolor(int c);
void con_bgcolor(int c);
void con_setattr(unsigned char attr);
unsigned char con_getattr(void);
void con_clear(void);
void con_putchar(int c);

void con_putchar_scr(int x, int y, int c);
int con_printf(int x, int y, const char *fmt, ...);

#endif	/* CONTTY_H_ */
