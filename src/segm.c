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
#include <string.h>
#include "segm.h"
#include "desc.h"
#include "tss.h"

/* bits for the 3rd 16bt part of the descriptor */
#define BIT_ACCESSED	(1 << 8)
#define BIT_WR			(1 << 9)
#define BIT_RD			(1 << 9)
#define BIT_EXP_DOWN	(1 << 10)
#define BIT_CONFORMING	(1 << 10)
#define BIT_CODE		(1 << 11)
#define BIT_NOSYS		(1 << 12)
#define BIT_PRESENT		(1 << 15)
/* TSS busy bit */
#define BIT_BUSY		(1 << 9)

/* bits for the last 16bit part of the descriptor */
#define BIT_BIG			(1 << 6)
#define BIT_DEFAULT		(1 << 6)
#define BIT_GRAN		(1 << 7)

enum {TYPE_DATA, TYPE_CODE};

/* we need the following bit pattern at the 8th bit excluding the busy bit: 1001 */
#define TSS_TYPE_BITS	(9 << 8)

static void segm_desc(desc_t *desc, uint32_t base, uint32_t limit, int dpl, int type);
static void segm_desc16(desc_t *desc, uint32_t base, uint32_t limit, int dpl, int type);
static void task_desc(desc_t *desc, uint32_t base, uint32_t limit, int dpl);

/* these functions are implemented in segm-asm.S */
void setup_selectors(uint16_t code, uint16_t data);
void set_gdt(uint32_t addr, uint16_t limit);
void set_task_reg(uint16_t tss_selector);


/* our global descriptor table */
static desc_t gdt[NUM_SEGMENTS] __attribute__((aligned(8)));


void init_segm(void)
{
	memset(gdt, 0, sizeof gdt);
	segm_desc(gdt + SEGM_KCODE, 0, 0xffffffff, 0, TYPE_CODE);
	segm_desc(gdt + SEGM_KDATA, 0, 0xffffffff, 0, TYPE_DATA);
	segm_desc(gdt + SEGM_UCODE, 0, 0xffffffff, 3, TYPE_CODE);
	segm_desc(gdt + SEGM_UDATA, 0, 0xffffffff, 3, TYPE_DATA);
	segm_desc16(gdt + SEGM_CODE16, 0, 0xffff, 0, TYPE_CODE);

	set_gdt((uint32_t)gdt, sizeof gdt - 1);

	setup_selectors(selector(SEGM_KCODE, 0), selector(SEGM_KDATA, 0));
}

/* constructs a GDT selector based on index and priviledge level */
uint16_t selector(int idx, int rpl)
{
	return (idx << 3) | (rpl & 3);
}

void set_tss(uint32_t addr)
{
	task_desc(gdt + SEGM_TASK, addr, sizeof(struct task_state) - 1, 3);
	set_task_reg(selector(SEGM_TASK, 0));
}

static void segm_desc(desc_t *desc, uint32_t base, uint32_t limit, int dpl, int type)
{
	desc->d[0] = limit & 0xffff; /* low order 16bits of limit */
	desc->d[1] = base & 0xffff;  /* low order 16bits of base */

	/* third 16bit part contains the last 8 bits of base, the 2 priviledge
	 * level bits starting on bit 13, present flag on bit 15, and type bits
	 * starting from bit 8
	 */
	desc->d[2] = ((base >> 16) & 0xff) | ((dpl & 3) << 13) | BIT_PRESENT |
		BIT_NOSYS | (type == TYPE_DATA ? BIT_WR : (BIT_RD | BIT_CODE));

	/* last 16bit part contains the last nibble of limit, the last byte of
	 * base, and the granularity and deafult/big flags in bits 23 and 22 resp.
	 */
	desc->d[3] = ((limit >> 16) & 0xf) | ((base >> 16) & 0xff00) | BIT_GRAN | BIT_BIG;
}

static void segm_desc16(desc_t *desc, uint32_t base, uint32_t limit, int dpl, int type)
{
	desc->d[0] = limit & 0xffff; /* low order 16bits of limit */
	desc->d[1] = base & 0xffff;  /* low order 16bits of base */

	/* third 16bit part contains the last 8 bits of base, the 2 priviledge
	 * level bits starting on bit 13, present flag on bit 15, and type bits
	 * starting from bit 8
	 */
	desc->d[2] = ((base >> 16) & 0xff) | ((dpl & 3) << 13) | BIT_PRESENT |
		BIT_NOSYS | (type == TYPE_DATA ? BIT_WR : (BIT_RD | BIT_CODE));

	/* 16bit descriptors have the upper word 0 */
	desc->d[3] = 0;
}

static void task_desc(desc_t *desc, uint32_t base, uint32_t limit, int dpl)
{
	desc->d[0] = limit & 0xffff;
	desc->d[1] = base & 0xffff;

	desc->d[2] = ((base >> 16) & 0xff) | ((dpl & 3) << 13) | BIT_PRESENT |
		TSS_TYPE_BITS; /* XXX busy ? */
	desc->d[3] = ((limit >> 16) & 0xf) | ((base >> 16) & 0xff00) | BIT_GRAN;
}
/*
static void dbg_print_gdt(void)
{
	int i;

	printf("Global Descriptor Table\n");
	printf("-----------------------\n");

	for(i=0; i<6; i++) {
		print_desc(gdt + i);
	}
}

static void print_desc(desc_t *desc)
{
	uint32_t base, limit;
	int dpl, g, db, l, avl, p, s, type;
	char *type_str;

	base = (uint32_t)desc->d[1] | ((uint32_t)(desc->d[2] & 0xff) << 16) | ((uint32_t)(desc->d[3] >> 8) << 24);
	limit = (uint32_t)desc->d[0] | ((uint32_t)(desc->d[3] & 0xf) << 16);
	dpl = (desc->d[2] >> 13) & 3;
	type = (desc->d[2] >> 8) & 0xf;
	g = (desc->d[3] >> 23) & 1;
	db = (desc->d[3] >> 22) & 1;
	l = (desc->d[3] >> 21) & 1;
	avl = (desc->d[3] >> 20) & 1;

	p = (desc->d[2] >> 15) & 1;
	s = (desc->d[2] >> 12) & 1;
}
*/
