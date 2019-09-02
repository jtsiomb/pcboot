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
#ifndef KBSCAN_H_
#define KBSCAN_H_

/* table with rough translations from set 1 scancodes to ASCII-ish */
static int scantbl_set1[] = {
	0, KB_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',		/* 0 - e */
	'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',			/* f - 1c */
	KB_LCTRL, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',				/* 1d - 29 */
	KB_LSHIFT, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', KB_RSHIFT,			/* 2a - 36 */
	KB_NUM_MUL, KB_LALT, ' ', KB_CAPSLK, KB_F1, KB_F2, KB_F3, KB_F4, KB_F5, KB_F6, KB_F7, KB_F8, KB_F9, KB_F10,			/* 37 - 44 */
	KB_NUMLK, KB_SCRLK, KB_NUM_7, KB_NUM_8, KB_NUM_9, KB_NUM_MINUS, KB_NUM_4, KB_NUM_5, KB_NUM_6, KB_NUM_PLUS,	/* 45 - 4e */
	KB_NUM_1, KB_NUM_2, KB_NUM_3, KB_NUM_0, KB_NUM_DOT, KB_SYSRQ, 0, 0, KB_F11, KB_F12,						/* 4d - 58 */
	0, 0, 0, 0, 0, 0, 0,															/* 59 - 5f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,									/* 60 - 6f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0									/* 70 - 7f */
};

/* extended scancodes, after the 0xe0 prefix */
static int scantbl_set1_ext[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			/* 0 - f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\r', KB_RCTRL, 0, 0,			/* 10 - 1f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			/* 20 - 2f */
	0, 0, 0, 0, 0, KB_NUM_MINUS, 0, KB_SYSRQ, KB_RALT, 0, 0, 0, 0, 0, 0, 0,			/* 30 - 3f */
	0, 0, 0, 0, 0, 0, 0, KB_HOME, KB_UP, KB_PGUP, 0, KB_LEFT, 0, KB_RIGHT, 0, KB_END,	/* 40 - 4f */
	KB_DOWN, KB_PGDN, KB_INSERT, KB_DEL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			/* 50 - 5f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			/* 60 - 6f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,			/* 70 - 7f */
};


#endif	/* KBSCAN_H_ */
