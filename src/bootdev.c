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
#include "bootdev.h"
#include "boot.h"
#include "int86.h"
#include "panic.h"
#include "timer.h"
#include "floppy.h"

#define FLOPPY_MOTOR_OFF_TIMEOUT	4000
#define DBG_RESET_ON_FAIL

struct chs {
	unsigned int cyl, head, tsect;
};

struct disk_access {
	unsigned char pktsize;
	unsigned char zero;
	uint16_t num_sectors;	/* max 127 on some implementations */
	uint16_t boffs, bseg;
	uint32_t lba_low, lba_high;
} __attribute__((packed));

enum {OP_READ, OP_WRITE};

#ifdef DBG_RESET_ON_FAIL
static int bios_reset_dev(int dev);
#endif
static int bios_rw_sect_lba(int dev, uint64_t lba, int nsect, int op, void *buf);
static int bios_rw_sect_chs(int dev, struct chs *chs, int nsect, int op, void *buf);
static int get_drive_chs(int dev, struct chs *chs);
static void calc_chs(uint64_t lba, struct chs *chs);

static int have_bios_ext;
static int bdev_is_floppy;
static int num_cyl, num_heads, num_track_sect;

void bdev_init(void)
{
	struct chs chs;
	struct int86regs regs;

	memset(&regs, 0, sizeof regs);
	regs.eax = 0x4100;	/* function 41h: check int 13h extensions */
	regs.ebx = 0x55aa;
	regs.edx = boot_drive_number;

	int86(0x13, &regs);
	if((regs.flags & FLAGS_CARRY) || (regs.ecx & 1) == 0) {
		printf("BIOS does not support int13h extensions (LBA access)\n");
		have_bios_ext = 0;

		if(get_drive_chs(boot_drive_number, &chs) == -1) {
			panic("drive (%d) parameter query failed\n", boot_drive_number);
		}

		num_cyl = chs.cyl;
		num_heads = chs.head;
		num_track_sect = chs.tsect;

		printf("Drive params: %d cyl, %d heads, %d sect/track\n", num_cyl,
				num_heads, num_track_sect);
	} else {
		printf("BIOS supports int13h extensions (LBA access)\n");
		have_bios_ext = 1;
	}

	bdev_is_floppy = !(boot_drive_number & 0x80);
}

#define NRETRIES	3

int bdev_read_sect(uint64_t lba, void *buf)
{
	int i;
	struct chs chs;

	if(bdev_is_floppy) {
		cancel_alarm(floppy_motors_off);
		set_alarm(FLOPPY_MOTOR_OFF_TIMEOUT, floppy_motors_off);
	}

	if(have_bios_ext) {
		return bios_rw_sect_lba(boot_drive_number, lba, 1, OP_READ, buf);
	}

	calc_chs(lba, &chs);

	for(i=0; i<NRETRIES; i++) {
		if(bios_rw_sect_chs(boot_drive_number, &chs, 1, OP_READ, buf) != -1) {
			return 0;
		}
#ifdef DBG_RESET_ON_FAIL
		bios_reset_dev(boot_drive_number);
#endif
	}
	return -1;
}

int bdev_write_sect(uint64_t lba, void *buf)
{
	struct chs chs;

	if(bdev_is_floppy) {
		cancel_alarm(floppy_motors_off);
		set_alarm(FLOPPY_MOTOR_OFF_TIMEOUT, floppy_motors_off);
	}

	if(have_bios_ext) {
		return bios_rw_sect_lba(boot_drive_number, lba, 1, OP_WRITE, buf);
	}

	calc_chs(lba, &chs);
	return bios_rw_sect_chs(boot_drive_number, &chs, 1, OP_WRITE, buf);
}

int bdev_read_range(uint64_t lba, int nsect, void *buf)
{
	int i;
	struct chs chs;

	if(bdev_is_floppy) {
		cancel_alarm(floppy_motors_off);
		set_alarm(FLOPPY_MOTOR_OFF_TIMEOUT, floppy_motors_off);
	}

	if(have_bios_ext) {
		return bios_rw_sect_lba(boot_drive_number, lba, nsect, OP_READ, buf);
	}

	calc_chs(lba, &chs);

	for(i=0; i<NRETRIES; i++) {
		int rd;
		if((rd = bios_rw_sect_chs(boot_drive_number, &chs, nsect, OP_READ, buf)) == nsect) {
			return 0;
		}

		if(rd > 0) {
			nsect -= rd;
			buf = (char*)buf + rd * 512;
			lba += rd;
			calc_chs(lba, &chs);
		}

#ifdef DBG_RESET_ON_FAIL
		bios_reset_dev(boot_drive_number);
#endif
	}
	return -1;
}

int bdev_write_range(uint64_t lba, int nsect, void *buf)
{
	struct chs chs;

	if(bdev_is_floppy) {
		cancel_alarm(floppy_motors_off);
		set_alarm(FLOPPY_MOTOR_OFF_TIMEOUT, floppy_motors_off);
	}

	if(have_bios_ext) {
		return bios_rw_sect_lba(boot_drive_number, lba, nsect, OP_WRITE, buf);
	}

	calc_chs(lba, &chs);
	return bios_rw_sect_chs(boot_drive_number, &chs, nsect, OP_WRITE, buf);
}


#ifdef DBG_RESET_ON_FAIL
static int bios_reset_dev(int dev)
{
	struct int86regs regs;

	printf("resetting drive %d ...", dev);

	memset(&regs, 0, sizeof regs);
	regs.edx = dev;

	int86(0x13, &regs);

	printf("%s\n", (regs.flags & FLAGS_CARRY) ? "failed" : "done");

	return (regs.flags & FLAGS_CARRY) ? -1 : 0;
}
#endif

static int bios_rw_sect_lba(int dev, uint64_t lba, int nsect, int op, void *buf)
{
	struct int86regs regs;
	struct disk_access *dap = (struct disk_access*)low_mem_buffer;
	uint32_t addr = (uint32_t)low_mem_buffer;
	uint32_t xaddr = (addr + sizeof *dap + 15) & 0xfffffff0;
	void *xbuf = (void*)xaddr;
	int func;

	if(op == OP_READ) {
		func = 0x42;	/* function 42h: extended read sector (LBA) */
	} else {
		func = 0x43;	/* function 43h: extended write sector (LBA) */
		memcpy(xbuf, buf, nsect * 512);
	}

	dap->pktsize = sizeof *dap;
	dap->zero = 0;
	dap->num_sectors = nsect;
	dap->boffs = 0;
	dap->bseg = xaddr >> 4;
	dap->lba_low = (uint32_t)lba;
	dap->lba_high = (uint32_t)(lba >> 32);

	memset(&regs, 0, sizeof regs);
	regs.eax = func << 8;
	regs.ds = addr >> 4;
	regs.esi = 0;
	regs.edx = dev;

	int86(0x13, &regs);

	if(regs.flags & FLAGS_CARRY) {
		return -1;
	}

	if(op == OP_READ) {
		memcpy(buf, xbuf, nsect * 512);
	}
	return dap->num_sectors;
}

/* TODO: fix: probably can't cross track boundaries, clamp nsect accordingly
 * and return a short count
 */
static int bios_rw_sect_chs(int dev, struct chs *chs, int nsect, int op, void *buf)
{
	struct int86regs regs;
	uint32_t xaddr = (uint32_t)low_mem_buffer;
	int func;

	if(op == OP_READ) {
		func = 2;
	} else {
		func = 3;
		memcpy(low_mem_buffer, buf, nsect * 512);
	}

	memset(&regs, 0, sizeof regs);
	regs.eax = (func << 8) | nsect;	/* 1 sector */
	regs.es = xaddr >> 4;	/* es:bx buffer */
	regs.ecx = ((chs->cyl << 8) & 0xff00) | ((chs->cyl >> 10) & 0xc0) | chs->tsect;
	regs.edx = dev | (chs->head << 8);

	int86(0x13, &regs);

	if(regs.flags & FLAGS_CARRY) {
		return -1;
	}

	if(op == OP_READ) {
		memcpy(buf, low_mem_buffer, nsect * 512);
	}
	return nsect;
}

static int get_drive_chs(int dev, struct chs *chs)
{
	struct int86regs regs;

	memset(&regs, 0, sizeof regs);
	regs.eax = 0x800;
	regs.edx = dev;

	int86(0x13, &regs);

	if(regs.flags & FLAGS_CARRY) {
		return -1;
	}

	chs->cyl = (((regs.ecx >> 8) & 0xff) | ((regs.ecx << 2) & 0x300)) + 1;
	chs->head = (regs.edx >> 8) + 1;
	chs->tsect = regs.ecx & 0x3f;
	return 0;
}

static void calc_chs(uint64_t lba, struct chs *chs)
{
	uint32_t lba32, trk;

	if(lba >> 32) {
		/* XXX: 64bit ops require libgcc, and I don't want to link that for
		 * this. CHS I/O is only going to be for floppies and really old systems
		 * anyway, so there's no point in supporting anything more than LBA32 in
		 * the translation.
		 */
		const char *fmt = "calc_chs only supports 32bit LBA. requested: %llx\n"
			"If you see this message, please file a bug report at:\n"
			"  https://github.com/jtsiomb/256boss/issues\n"
			"  or by email: nuclear@member.fsf.org\n";
		panic(fmt, lba);
	}

	lba32 = (uint32_t)lba;
	trk = lba32 / num_track_sect;
	chs->tsect = (lba32 % num_track_sect) + 1;
	chs->cyl = trk / num_heads;
	chs->head = trk % num_heads;
}
