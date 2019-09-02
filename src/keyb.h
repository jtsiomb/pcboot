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
#ifndef KEYB_H_
#define KEYB_H_

#define KB_ANY		(-1)
#define KB_ALT		(-2)
#define KB_CTRL		(-3)
#define KB_SHIFT	(-4)

/* special keys */
enum {
	KB_ESC = 27,
	KB_INSERT, KB_DEL, KB_HOME, KB_END, KB_PGUP, KB_PGDN,
	KB_LEFT, KB_RIGHT, KB_UP, KB_DOWN,
	KB_NUM_DOT, KB_NUM_ENTER, KB_NUM_PLUS, KB_NUM_MINUS, KB_NUM_MUL, KB_NUM_DIV,
	KB_NUM_0, KB_NUM_1, KB_NUM_2, KB_NUM_3, KB_NUM_4,
	KB_NUM_5, KB_NUM_6, KB_NUM_7, KB_NUM_8, KB_NUM_9,
	KB_BACKSP = 127,

	KB_LALT, KB_RALT,
	KB_LCTRL, KB_RCTRL,
	KB_LSHIFT, KB_RSHIFT,
	KB_F1, KB_F2, KB_F3, KB_F4, KB_F5, KB_F6,
	KB_F7, KB_F8, KB_F9, KB_F10, KB_F11, KB_F12,
	KB_CAPSLK, KB_NUMLK, KB_SCRLK, KB_SYSRQ
};

void kb_init(void);

void kb_intr_enable(void);
void kb_intr_disable(void);

int kb_setmode(int mode);
int kb_getmode(void);

void kb_set_translate(int xlat);
int kb_get_translate(void);

/* Boolean predicate for testing the current state of a particular key.
 * You may also pass KB_ANY to test if any key is held down.
 */
int kb_isdown(int key);

/* waits for any keypress */
void kb_wait(void);

/* removes and returns a single key from the input buffer. */
int kb_getkey(void);

void kb_putback(int key);

/* returns 1 if the keyboard controller is ready to read/write
 * returns 0 if the wait times out
 */
int kb_wait_write(void);
int kb_wait_read(void);

void kb_send_cmd(unsigned char cmd);
void kb_send_data(unsigned char data);
unsigned char kb_read_data(void);

#endif	/* KEYB_H_ */
