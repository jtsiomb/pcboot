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
#include <inttypes.h>
#include "pci.h"
#include "int86.h"
#include "asmops.h"
#include "panic.h"

#define CONFIG_ADDR_PORT	0xcf8
#define CONFIG_DATA_PORT	0xcfc

#define ADDR_ENABLE		0x80000000
#define ADDR_BUSID(x)	(((uint32_t)(x) & 0xff) << 16)
#define ADDR_DEVID(x)	(((uint32_t)(x) & 0x1f) << 11)
#define ADDR_FUNC(x)	(((uint32_t)(x) & 3) << 8)

/* signature returned in edx by the PCI BIOS present function: FOURCC "PCI " */
#define PCI_SIG		0x20494350

static uint16_t cfg_read32_acc1(int bus, int dev, int func, int reg);
static uint16_t cfg_read32_acc2(int bus, int dev, int func, int reg);

static uint16_t (*cfg_read32)(int, int, int, int);

void init_pci(void)
{
	struct int86regs regs;

	memset(&regs, 0, sizeof regs);
	regs.eax = 0xb101;
	int86(0x1a, &regs);

	/* PCI BIOS present if CF=0, AH=0, and EDX has the "PCI " sig FOURCC */
	if((regs.flags & FLAGS_CARRY) || (regs.eax & 0xff00) || regs.edx != PCI_SIG) {
		printf("No PCI BIOS present\n");
		return;
	}

	printf("PCI BIOS v%x.%x found\n", (regs.ebx & 0xff00) >> 8, regs.ebx & 0xff);
	if(regs.eax & 1) {
		cfg_read32 = cfg_read32_acc1;
	} else {
		if(!(regs.eax & 2)) {
			printf("Failed to find supported PCI access mechanism\n");
			return;
		}
		printf("PCI access mechanism #1 unsupported, falling back to mechanism #2\n");
		cfg_read32 = cfg_read32_acc2;
	}
}

static uint16_t cfg_read32_acc1(int bus, int dev, int func, int reg)
{
	uint32_t addr = ADDR_ENABLE | ADDR_BUSID(bus) | ADDR_DEVID(dev) |
		ADDR_FUNC(func) | reg;

	outl(addr, CONFIG_ADDR_PORT);
	return inl(CONFIG_DATA_PORT);
}

static uint16_t cfg_read32_acc2(int bus, int dev, int func, int reg)
{
	panic("BUG: PCI access mechanism #2 not implemented yet!");
	return 0;
}
