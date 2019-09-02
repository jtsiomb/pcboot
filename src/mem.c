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
#include "config.h"
#include "panic.h"
#include "mem.h"
#include "intr.h"

#define FREE		0
#define USED		1

#define BM_IDX(pg)	((pg) / 32)
#define BM_BIT(pg)	((pg) & 0x1f)

#define IS_FREE(pg) ((bitmap[BM_IDX(pg)] & (1 << BM_BIT(pg))) == 0)

#define MEM_START	((uint32_t)&_mem_start)


struct mem_range {
	uint32_t start;
	uint32_t size;
};

void move_stack(uint32_t newaddr);	/* defined in startup.s */

static void mark_page(int pg, int used);
static void add_memory(uint32_t start, size_t size);

#define MAX_MAP_SIZE	16
extern struct mem_range boot_mem_map[MAX_MAP_SIZE];
extern int boot_mem_map_size;

/* linker supplied symbol, points to the end of the kernel image */
extern uint32_t _mem_start;


/* A bitmap is used to track which physical memory pages are used, and which
 * are available for allocation by alloc_ppage.
 *
 * last_alloc_idx keeps track of the last 32bit element in the bitmap array
 * where a free page was found. It's guaranteed that all the elements before
 * this have no free pages, but it doesn't imply that there will be another
 * free page there. So it's used as a starting point for the search.
 */
static uint32_t *bitmap;
static int bmsize, last_alloc_idx;



void init_mem(void)
{
	int i, pg, max_used_pg, end_pg = 0;
	uint32_t used_end, start, end, sz, total = 0, rem;
	const char *suffix[] = {"bytes", "KB", "MB", "GB"};

	last_alloc_idx = 0;

	/* the allocation bitmap starts right at the end of the kernel image */
	bitmap = (uint32_t*)&_mem_start;

	/* start by marking all posible pages (2**20) as used. We do not "reserve"
	 * all this space. Pages beyond the end of the useful bitmap area
	 * ((char*)bitmap + bmsize), which will be determined after we traverse the
	 * memory map, are going to be marked as available for allocation.
	 */
	memset(bitmap, 0xff, 1024 * 1024 / 8);


	if(boot_mem_map_size <= 0 || boot_mem_map_size > MAX_MAP_SIZE) {
		panic("invalid memory map size reported by the boot loader: %d\n", boot_mem_map_size);
	}

	printf("Memory map:\n");
	for(i=0; i<boot_mem_map_size; i++) {
		printf(" start: %08x - size: %08x\n", (unsigned int)boot_mem_map[i].start,
				(unsigned int)boot_mem_map[i].size);

		start = boot_mem_map[i].start;
		sz = boot_mem_map[i].size & 0xfffffffc;
		end = start + sz;
		if(end < sz) {
			end = 0xfffffffc;
			sz = end - start;
		}

		/* ignore any memory ranges which end before the end of the kernel image */
		if(end < MEM_START) continue;
		if(start < MEM_START) {
			start = MEM_START;
		}

		add_memory(start, sz);
		total += sz;

		pg = ADDR_TO_PAGE(end);
		if(end_pg < pg) {
			end_pg = pg;
		}
	}

	i = 0;
	while(total > 1024) {
		rem = total & 0x3ff;
		total >>= 10;
		++i;
	}
	printf("Total usable RAM: %u.%u %s\n", total, 100 * rem / 1024, suffix[i]);

	/* size of the useful part of the bitmap in bytes padded to 4-byte
	 * boundaries to allow 32bit at a time operations.
	 */
	bmsize = (end_pg / 32) * 4;

	/* mark all pages occupied by the bitmap as used */
	used_end = (uint32_t)bitmap + bmsize - 1;

	max_used_pg = ADDR_TO_PAGE(used_end);
	printf("marking pages up to %x (page: %d) as used\n", used_end, max_used_pg);
	for(i=0; i<=max_used_pg; i++) {
		mark_page(i, USED);
	}

#ifdef MOVE_STACK_RAMTOP
	/* allocate space for the stack at the top of RAM and move it there */
	if((pg = alloc_ppages(STACK_PAGES, MEM_STACK)) != -1) {
		printf("moving stack-top to: %x (%d pages)\n", PAGE_TO_ADDR(end_pg) - 4, STACK_PAGES);
		move_stack(PAGE_TO_ADDR(pg + STACK_PAGES) - 4);
	}
#endif
}

int alloc_ppage(int area)
{
	return alloc_ppages(1, area);
}

/* free_ppage marks the physical page, free in the allocation bitmap.
 *
 * CAUTION: no checks are done that this page should actually be freed or not.
 * If you call free_ppage with the address of some part of memory that was
 * originally reserved due to it being in a memory hole or part of the kernel
 * image or whatever, it will be subsequently allocatable by alloc_ppage.
 */
void free_ppage(int pg)
{
	int bmidx = BM_IDX(pg);

	int intr_state = get_intr_flag();
	disable_intr();

	if(IS_FREE(pg)) {
		panic("free_ppage(%d): I thought that was already free!\n", pg);
	}

	mark_page(pg, FREE);
	if(bmidx < last_alloc_idx) {
		last_alloc_idx = bmidx;
	}

	set_intr_flag(intr_state);
}


int alloc_ppages(int count, int area)
{
	int i, dir, pg, idx, max, intr_state, found_free = 0;

	intr_state = get_intr_flag();
	disable_intr();

	if(area == MEM_STACK) {
		idx = (bmsize - 1) / 4;
		max = -1;
		dir = -1;
	} else {
		idx = last_alloc_idx;
		max = bmsize / 4;
		dir = 1;
	}

	while(idx != max) {
		/* if at least one bit is 0 then we have at least
		 * one free page. find it and try to allocate a range starting from there
		 */
		if(bitmap[idx] != 0xffffffff) {
			pg = idx * 32;
			if(dir < 0) pg += 31;

			for(i=0; i<32; i++) {
				if(IS_FREE(pg)) {
					if(!found_free && dir > 0) {
						last_alloc_idx = idx;
						found_free = 1;
					}

					if(alloc_ppage_range(pg, count) != -1) {
						set_intr_flag(intr_state);
						return pg;
					}
				}
				pg += dir;
			}
		}
		idx += dir;
	}

	set_intr_flag(intr_state);
	return -1;
}

void free_ppages(int pg0, int count)
{
	int i;

	for(i=0; i<count; i++) {
		free_ppage(pg0++);
	}
}

int alloc_ppage_range(int start, int size)
{
	int i, pg = start;
	int intr_state;

	intr_state = get_intr_flag();
	disable_intr();

	/* first validate that no page in the requested range is allocated */
	for(i=0; i<size; i++) {
		if(!IS_FREE(pg)) {
			return -1;
		}
		++pg;
	}

	/* all is well, mark them as used */
	pg = start;
	for(i=0; i<size; i++) {
		mark_page(pg++, USED);
	}

	set_intr_flag(intr_state);
	return 0;
}

int free_ppage_range(int start, int size)
{
	int i, pg = start;

	for(i=0; i<size; i++) {
		free_ppage(pg++);
	}
	return 0;
}

/* adds a range of physical memory to the available pool. used during init_mem
 * when traversing the memory map.
 */
static void add_memory(uint32_t start, size_t sz)
{
	int i, szpg, pg;

	szpg = ADDR_TO_PAGE(sz + 4095);
	pg = ADDR_TO_PAGE(start);

	for(i=0; i<szpg; i++) {
		mark_page(pg++, FREE);
	}
}

/* maps a page as used or free in the allocation bitmap */
static void mark_page(int pg, int used)
{
	int idx = BM_IDX(pg);
	int bit = BM_BIT(pg);

	if(used) {
		bitmap[idx] |= 1 << bit;
	} else {
		bitmap[idx] &= ~(1 << bit);
	}
}

void print_page_bitmap(void)
{
	int i;

	for(i=0; i<bmsize/4; i++) {
		if((i & 3) == 0) {
			uint32_t pg = i * 32;
			uint32_t addr = PAGE_TO_ADDR(pg);
			printf("\n%5d [%08x]:", (int)pg, (unsigned long)addr);
		}
		printf(" %08x", bitmap[i]);
	}
	printf("\n");
}
