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
#include "psaux.h"
#include "intr.h"
#include "asmops.h"
#include "keyb.h"
#include "kbregs.h"

static void init_mouse();
static void psaux_intr();

static int mx, my;
static unsigned int bnstate;
static int present;
static int bounds[4];

void init_psaux(void)
{
	interrupt(IRQ_TO_INTR(12), psaux_intr);

	set_mouse_bounds(0, 0, 319, 199);

	init_mouse();
}

static void init_mouse()
{
	unsigned char val;

	kb_send_cmd(KB_CMD_AUX_ENABLE);
	if(kb_wait_read()) {
		val = kb_read_data();
		printf("aux enable: %02x\n", (unsigned int)val);
	}

	kb_send_cmd(KB_CMD_GET_CMDBYTE);
	val = kb_read_data();
	val &= ~KB_CCB_AUX_DISABLE;
	val |= KB_CCB_AUX_INTREN;
	kb_send_cmd(KB_CMD_SET_CMDBYTE);
	kb_send_data(val);

	if(kb_wait_read()) {
		val = kb_read_data();
		printf("set cmdbyte: %02x\n", (unsigned int)val);
	}

	kb_send_cmd(KB_CMD_PSAUX);
	kb_send_data(AUX_CMD_DEFAULTS);
	val = kb_read_data();

	kb_send_cmd(KB_CMD_PSAUX);
	kb_send_data(AUX_CMD_ENABLE);
	val = kb_read_data();

	present = (val == KB_ACK) ? 1 : 0;
	printf("init_mouse: %spresent\n", present ? "" : "not ");
}

int have_mouse(void)
{
	return present;
}

void set_mouse_bounds(int x0, int y0, int x1, int y1)
{
	bounds[0] = x0;
	bounds[1] = y0;
	bounds[2] = x1;
	bounds[3] = y1;
}

unsigned int mouse_state(int *xp, int *yp)
{
	*xp = mx;
	*yp = my;
	return bnstate;
}

static void psaux_intr()
{
	static unsigned char data[3];
	static int idx;
	int dx, dy;

	if(!(inb(KB_STATUS_PORT) & KB_STAT_AUX)) {
		/* no mouse data pending, ignore interrupt */
		return;
	}

	data[idx] = kb_read_data();
	if(++idx >= 3) {
		idx = 0;

		if(data[0] & AUX_PKT0_OVF_BITS) {
			/* consensus seems to be that if overflow bits are set, something is
			 * fucked, and it's best to re-initialize the mouse
			 */
			init_mouse();
		} else {
			/*
			printf("psaux data packet: %02x %02x %02x\n", (unsigned int)data[0],
					(unsigned int)data[1], (unsigned int)data[2]);
			*/

			bnstate = data[0] & AUX_PKT0_BUTTON_BITS;
			dx = data[1];
			dy = data[2];

			if(data[0] & AUX_PKT0_XSIGN) {
				dx |= 0xffffff00;
			}
			if(data[0] & AUX_PKT0_YSIGN) {
				dy |= 0xffffff00;
			}

			mx += dx;
			my -= dy;

			if(mx < bounds[0]) mx = bounds[0];
			if(mx > bounds[2]) mx = bounds[2];
			if(my < bounds[1]) my = bounds[1];
			if(my > bounds[3]) my = bounds[3];

			/*
			printf("mouse: %d,%d [%c%c%c]\n", mx, my, bnstate & AUX_PKT0_LEFTBN ? '1' : '0',
					bnstate & AUX_PKT0_MIDDLEBN ? '1' : '0', bnstate & AUX_PKT0_RIGHTBN ? '1' : '0');
			*/
		}
	}
}
