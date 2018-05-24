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
#include "audio.h"
#include "au_sb.h"
#include "asmops.h"

#define REG_MIXPORT		(base_port + 0x4)
#define REG_MIXDATA		(base_port + 0x5)
#define REG_RESET		(base_port + 0x6)
#define REG_RDATA		(base_port + 0xa)
#define REG_WDATA		(base_port + 0xc)
#define REG_WSTAT		(base_port + 0xc)
#define REG_RSTAT		(base_port + 0xe)
#define REG_INTACK		(base_port + 0xe)
#define REG_INT16ACK	(base_port + 0xf)

#define WSTAT_BUSY		0x80
#define RSTAT_RDY		0x80

#define CMD_SET_RATE	0x41
#define CMD_PLAY_PCM	0xa6
#define CMD_STOP_PCM	0xd9
#define CMD_GET_VER		0xe1

#define VER_MAJOR(x)	((x) >> 8)
#define VER_MINOR(x)	((x) & 0xff)

static void write_dsp(unsigned char val);
static unsigned char read_dsp(void);
static int get_dsp_version(void);
static const char *sbname(int ver);

static int base_port;

int sb_detect(void)
{
	int i, ver;

	for(i=0; i<6; i++) {
		base_port = 0x200 + ((i + 1) << 4);
		if(sb_reset_dsp() == 0) {
			ver = get_dsp_version();
			printf("sb_detect: found %s (DSP v%d.%02d) at port %xh\n", sbname(ver),
					VER_MAJOR(ver), VER_MINOR(ver), base_port);
			return 1;
		}
	}

	return 0;
}

int sb_reset_dsp(void)
{
	int i;

	outb(1, REG_RESET);
	for(i=0; i<3; i++) iodelay();
	outb(0, REG_RESET);

	for(i=0; i<128; i++) {
		if(inb(REG_RSTAT) & RSTAT_RDY) {
			if(inb(REG_RDATA) == 0xaa) {
				return 0;
			}
		}
	}

	return -1;
}

static void write_dsp(unsigned char val)
{
	while(inb(REG_WSTAT) & WSTAT_BUSY);
	outb(val, REG_WDATA);
}

static unsigned char read_dsp(void)
{
	while((inb(REG_RSTAT) & RSTAT_RDY) == 0);
	return inb(REG_RDATA);
}

static int get_dsp_version(void)
{
	int major, minor;

	write_dsp(CMD_GET_VER);
	major = read_dsp();
	minor = read_dsp();

	return (major << 8) | minor;
}

#define V(maj, min)	(((maj) << 8) | (min))

static const char *sbname(int ver)
{
	int major = VER_MAJOR(ver);
	int minor = VER_MINOR(ver);

	switch(major) {
	case 1:
		if(minor == 5) {
			return "Sound Blaster 1.5";
		}
		return "Sound Blaster 1.0";

	case 2:
		if(minor == 1 || minor == 2) {
			return "Sound Blaster 2.0";
		}
		break;

	case 3:
		switch(minor) {
		case 0:
			return "Sound Blaster Pro";
		case 1:
		case 2:
			return "Sound Blaster Pro 2";
		case 5:
			return "Gallant SC-6000";
		default:
			break;
		}
		break;

	case 4:
		switch(minor) {
		case 4:
		case 5:
			return "Sound Blaster 16";
		case 11:
			return "Sound Blaster 16 SCSI-2";
		case 12:
			return "Sound Blaster AWE 32";
		case 13:
			return "Sound Blaster ViBRA16C";
		case 16:
			return "Sound Blaster AWE 64";
		default:
			break;
		}
		break;
	}

	return "Unknown Sound Blaster";
}
