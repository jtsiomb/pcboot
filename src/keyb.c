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
#include "keyb.h"
#include "intr.h"
#include "asmops.h"
#include "kbregs.h"
#include "kbscan.h"
#include "power.h"
#include "panic.h"

#define delay7us() \
	do { \
		iodelay(); iodelay(); iodelay(); iodelay(); \
		iodelay(); iodelay(); iodelay(); \
	} while(0)

static void set_ccb(unsigned char ccb);
static unsigned char get_ccb(void);

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

	/* make sure set1 translation is enabled */
	kb_set_translate(1);

	interrupt(IRQ_TO_INTR(KB_IRQ), kbintr);
	kb_intr_enable();
}

void kb_intr_enable(void)
{
	unsigned char ccb = get_ccb();
	ccb |= KB_CCB_KB_INTREN;
	set_ccb(ccb);
}

void kb_intr_disable(void)
{
	unsigned char ccb = get_ccb();
	ccb &= ~KB_CCB_KB_INTREN;
	set_ccb(ccb);
}

int kb_setmode(int mode)
{
	kb_send_data(0xf0);
	if(!kb_wait_read() || kb_read_data() != KB_ACK) {
		return -1;
	}
	kb_send_data(mode);
	if(!kb_wait_read() || kb_read_data() != KB_ACK) {
		return -1;
	}
	return 0;
}

int kb_getmode(void)
{
	int mode;

	kb_send_data(0xf0);
	if(!kb_wait_read() || kb_read_data() != KB_ACK) {
		return -1;
	}
	kb_send_data(0);
	if(!kb_wait_read() || kb_read_data() != KB_ACK) {
		return -1;
	}
	mode = kb_read_data();

	switch(mode) {
	case 0x43: return 1;
	case 0x41: return 2;
	case 0x3f: return 3;
	default:
		break;
	}
	return mode;
}

void kb_set_translate(int xlat)
{
	unsigned char ccb = get_ccb();
	if(xlat) {
		ccb |= KB_CCB_KB_XLAT;
	} else {
		ccb &= ~KB_CCB_KB_XLAT;
	}
	set_ccb(ccb);
}

int kb_get_translate(void)
{
	return get_ccb() & KB_CCB_KB_XLAT;
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

static void set_ccb(unsigned char ccb)
{
	kb_send_cmd(KB_CMD_SET_CMDBYTE);
	kb_send_data(ccb);

	if(kb_wait_read()) {
		kb_read_data();
	}
}

static unsigned char get_ccb(void)
{
	kb_send_cmd(KB_CMD_GET_CMDBYTE);
	return kb_read_data();
}

static void kbintr()
{
	unsigned char code;
	int key, press;
	static int ext = 0;

	code = inb(KB_DATA_PORT);

	if(code == 0xe0) {
		ext = 1;
		return;
	}

	if(code & 0x80) {
		press = 0;
		code &= 0x7f;

		if(num_pressed > 0) {
			num_pressed--;
		}
	} else {
		press = 1;

		num_pressed++;
	}

	key = ext ? scantbl_set1_ext[code] : scantbl_set1[code];
	ext = 0;

	if(press) {
		if(key == KB_DEL && (keystate[KB_LALT] || keystate[KB_RALT]) && (keystate[KB_LCTRL] || keystate[KB_RCTRL])) {
			reboot();
		}
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
