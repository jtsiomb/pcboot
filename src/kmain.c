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
#include <ctype.h>
#include "segm.h"
#include "intr.h"
#include "mem.h"
#include "keyb.h"
#include "psaux.h"
#include "timer.h"
#include "contty.h"
#include "video.h"
#include "audio.h"
#include "pci.h"
#include "vbetest.h"


void logohack(void);

void pcboot_main(void)
{
	init_segm();
	init_intr();

	con_init();
	kb_init();
	init_psaux();

	init_mem();

	init_pci();

	/* initialize the timer */
	init_timer();

	audio_init();

	enable_intr();

	printf("PCBoot kernel initialized\n");

	for(;;) {
		int c;

		halt_cpu();
		while((c = kb_getkey()) >= 0) {
			switch(c) {
			case KB_F1:
				set_vga_mode(0x13);
				logohack();
				set_vga_mode(3);
				break;

			case KB_F2:
				vbetest();
				break;
			}
			if(isprint(c)) {
				printf("key: %d '%c'\n", c, (char)c);
			} else {
				printf("key: %d\n", c);
			}
		}
		if((nticks % 250) == 0) {
			con_printf(71, 0, "[%ld]", nticks);
		}
	}
}
