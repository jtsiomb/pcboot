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
#include "keyb.h"
#include "intr.h"
#include "asmops.h"
#include "kbregs.h"

#define delay7us() \
	do { \
		iodelay(); iodelay(); iodelay(); iodelay(); \
		iodelay(); iodelay(); iodelay(); \
	} while(0)

/* table with rough translations from set 1 scancodes to ASCII-ish */
static int scantbl[] = {
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

static void kbintr();

#define BUFSZ	64
#define ADVANCE(x)	((x) = ((x) + 1) & (BUFSZ - 1))

static int buffer[BUFSZ];
static int buf_ridx, buf_widx;

static unsigned int num_pressed;
static unsigned char keystate[256];

void kb_init(void)
{
	buf_ridx = buf_widx = 0;
	num_pressed = 0;
	memset(keystate, 0, sizeof keystate);

	interrupt(IRQ_TO_INTR(KB_IRQ), kbintr);
}

int kb_isdown(int key)
{
	switch(key) {
	case KB_ANY:
		return num_pressed;

	case KB_ALT:
		return keystate[KB_LALT] + keystate[KB_RALT];

	case KB_CTRL:
		return keystate[KB_LCTRL] + keystate[KB_RCTRL];
	}
	return keystate[key];
}

void kb_wait(void)
{
	int key;
	while((key = kb_getkey()) == -1) {
		/* put the processor to sleep while waiting for keypresses, but first
		 * make sure interrupts are enabled, or we'll sleep forever
		 */
		enable_intr();
		halt_cpu();
	}
	kb_putback(key);
}

int kb_getkey(void)
{
	int res;

	if(buf_ridx == buf_widx) {
		return -1;
	}
	res = buffer[buf_ridx];
	ADVANCE(buf_ridx);
	return res;
}

void kb_putback(int key)
{
	/* go back a place */
	if(--buf_ridx < 0) {
		buf_ridx += BUFSZ;
	}

	/* if the write end hasn't caught up with us, go back one place
	 * and put it there, otherwise just overwrite the oldest key which
	 * is right where we were.
	 */
	if(buf_ridx == buf_widx) {
		ADVANCE(buf_ridx);
	}

	buffer[buf_ridx] = key;
}

int kb_wait_write(void)
{
	int i;
	for(i=0; i<32768; i++) {
		if(!(inb(KB_STATUS_PORT) & KB_STAT_INBUF_FULL)) {
			return 1;
		}
		iodelay();
	}
	/*printf("kb_wait_write timeout\n");*/
	return 0;
}

int kb_wait_read(void)
{
	int i;
	for(i=0; i<32768; i++) {
		if((inb(KB_STATUS_PORT) & KB_STAT_OUTBUF_FULL)) {
			return 1;
		}
		iodelay();
	}
	/*printf("kb_wait_read timeout\n");*/
	return 0;
}

void kb_send_cmd(unsigned char cmd)
{
	kb_wait_write();
	outb(cmd, KB_CMD_PORT);
}

void kb_send_data(unsigned char data)
{
	kb_wait_write();
	outb(data, KB_DATA_PORT);
}

unsigned char kb_read_data(void)
{
	kb_wait_read();
	delay7us();
	return inb(KB_DATA_PORT);
}

static void kbintr()
{
	unsigned char code;
	int key, press;

	code = inb(KB_DATA_PORT);

	if(code >= 128) {
		press = 0;
		code -= 128;

		if(num_pressed > 0) {
			num_pressed--;
		}
	} else {
		press = 1;

		num_pressed++;
	}

	key = scantbl[code];

	if(press) {
		/* append to buffer */
		buffer[buf_widx] = key;
		ADVANCE(buf_widx);
		/* if the write end overtook the read end, advance the read end
		 * too, to discard the oldest keypress from the buffer
		 */
		if(buf_widx == buf_ridx) {
			ADVANCE(buf_ridx);
		}
	}

	/* and update keystate table */
	keystate[key] = press;
}
