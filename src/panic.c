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
#include "asmops.h"

struct all_registers {
	uint32_t eax, ebx, ecx, edx;
	uint32_t esp, ebp, esi, edi;
	uint32_t eflags;
	uint32_t cs, ss, ds, es, fs, gs;
	uint32_t cr0, cr1, cr2, cr3;
};

/* defined in regs.S */
void get_regs(struct all_registers *regs);

void panic(const char *fmt, ...)
{
	va_list ap;
	struct all_registers regs;
	uint32_t eip;

	disable_intr();

	memset(&regs, 0, sizeof regs);
	get_regs(&regs);

	CALLER_EIP(eip);

	printf("~~~~~ pcboot panic ~~~~~\n");
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);

	printf("\nRegisters:\n");
	printf("eax: %x, ebx: %x, ecx: %x, edx: %x\n", regs.eax, regs.ebx, regs.ecx, regs.edx);
	printf("esp: %x, ebp: %x, esi: %x, edi: %x\n", regs.esp, regs.ebp, regs.esi, regs.edi);
	printf("eip: %x, eflags: %x\n", eip, regs.eflags);
	printf("cr0: %x, cr1: %x, cr2: %x, cr3: %x\n", regs.cr0, regs.cr1, regs.cr2, regs.cr3);
	printf("cs: %x (%d|%d)\n", regs.cs, regs.cs >> 3, regs.cs & 3);
	printf("ss: %x (%d|%d)\n", regs.ss, regs.ss >> 3, regs.ss & 3);
	printf("ds: %x (%d|%d)\n", regs.ds, regs.ds >> 3, regs.ds & 3);
	printf("es: %x (%d|%d)\n", regs.es, regs.es >> 3, regs.es & 3);
	printf("fs: %x (%d|%d)\n", regs.fs, regs.fs >> 3, regs.fs & 3);
	printf("gs: %x (%d|%d)\n", regs.gs, regs.gs >> 3, regs.gs & 3);

	for(;;) halt_cpu();
}
