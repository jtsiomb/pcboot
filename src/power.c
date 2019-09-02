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
#include "power.h"
#include "keyb.h"
#include "kbregs.h"
#include "timer.h"
#include "panic.h"
#include "asmops.h"

/* defined in intr_asm.S */
void set_idt(uint32_t addr, uint16_t limit);

void reboot(void)
{
	int i;

	printf("reboot: keyboard controller\n");
	kb_send_cmd(KB_CMD_PULSE_RESET);

	for(i=0; i<32768; i++) {
		iodelay();
	}

	printf("reboot: triple-fault\n");
	set_idt(0, 0);
	asm volatile("int $3");

	panic("can't reboot!");
}
