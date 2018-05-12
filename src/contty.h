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

int con_init(void);
void con_show_cursor(int show);
void con_cursor(int x, int y);
void con_fgcolor(int c);
void con_bgcolor(int c);
void con_clear(void);
void con_putchar(int c);

void con_putchar_scr(int x, int y, int c);
void con_printf(int x, int y, const char *fmt, ...);

#endif	/* CONTTY_H_ */
