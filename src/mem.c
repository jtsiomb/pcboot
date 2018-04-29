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
#include "panic.h"
#include "mem.h"

struct mem_range {
	uint32_t start;
	uint32_t size;
};

#define MAX_MAP_SIZE	16
extern struct mem_range boot_mem_map[MAX_MAP_SIZE];
extern int boot_mem_map_size;

void init_mem(void)
{
	int i;

	if(boot_mem_map_size <= 0 || boot_mem_map_size > MAX_MAP_SIZE) {
		panic("invalid memory map size reported by the boot loader: %d\n", boot_mem_map_size);
	}

	printf("Memory map:\n");
	for(i=0; i<boot_mem_map_size; i++) {
		printf(" start: %08x - size: %08x\n", (unsigned int)boot_mem_map[i].start,
				(unsigned int)boot_mem_map[i].size);
	}
}
