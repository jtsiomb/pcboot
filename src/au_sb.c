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
#include "intr.h"
#include "dma.h"

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

#define CMD_RATE			0x40
#define CMD_SB16_OUT_RATE	0x41
#define CMD_SB16_IN_RATE	0x42
#define CMD_GET_VER			0xe1

/* start DMA playback/recording. combine with fifo/auto/input flags */
#define CMD_START_DMA8		0xc0
#define CMD_START_DMA16		0xb0
#define CMD_FIFO			0x02
#define CMD_AUTO			0x04
#define CMD_INPUT			0x08

/* immediately pause/continue */
#define CMD_PAUSE_DMA8		0xd0
#define CMD_ENABLE_OUTPUT	0xd1
#define CMD_DISABLE_OUTPUT	0xd3
#define CMD_CONT_DMA8		0xd4
#define CMD_PAUSE_DMA16		0xd5
#define CMD_CONT_DMA16		0xd6

/* end the playback at the end of the current buffer */
#define CMD_END_DMA16		0xd9
#define CMD_END_DMA8		0xda

/* transfer mode commands */
#define CMD_MODE_SIGNED		0x10
#define CMD_MODE_STEREO		0x20

/* mixer registers */
#define MIX_IRQ_SEL			0x80
#define MIX_DMA_SEL			0x81

#define VER_MAJOR(x)	((x) >> 8)
#define VER_MINOR(x)	((x) & 0xff)

static void intr_handler();
static void start_dma_transfer(uint32_t addr, int size);
static void write_dsp(unsigned char val);
static unsigned char read_dsp(void);
static void write_mix(unsigned char val, int reg);
static unsigned char read_mix(int reg);
static int get_dsp_version(void);
static int sb16_detect_irq(void);
static int sb16_detect_dma(void);
static const char *sbname(int ver);

extern unsigned char low_mem_buffer[];

static int base_port;
static int irq, dma_chan, dma16_chan;
static int sb16;
static void *buffer;
static int xfer_mode;

int sb_detect(void)
{
	int i, ver;

	for(i=0; i<6; i++) {
		base_port = 0x200 + ((i + 1) << 4);
		if(sb_reset_dsp() == 0) {
			ver = get_dsp_version();
			sb16 = VER_MAJOR(ver) >= 4;

			if(sb16) {
				if(sb16_detect_irq() == -1) {
					printf("sb_detect: failed to configure IRQ\n");
					return 0;
				}
				if(sb16_detect_dma() == -1) {
					printf("sb_detect: failed to configure DMA\n");
					return 0;
				}

				printf("sb_detect: found %s (DSP v%d.%02d) at port %xh, irq %d, dma %d/%d\n",
						sbname(ver), VER_MAJOR(ver), VER_MINOR(ver),
						base_port, irq, dma_chan, dma16_chan);

			} else {
				/* XXX for old sound blasters, hard-code to IRQ 5 DMA 1 for now */
				irq = 5;
				dma_chan = 1;
				dma16_chan = -1;

				printf("sb_detect: found %s (DSP v%d.%02d) at port %xh\n", sbname(ver),
						VER_MAJOR(ver), VER_MINOR(ver), base_port);
				printf("sb_detect: old sound blaster dsp. assuming: irq 5, dma 1\n");
			}

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

void sb_set_output_rate(int rate)
{
	if(sb16) {
		write_dsp(CMD_SB16_OUT_RATE);
		write_dsp(rate >> 8);
		write_dsp(rate & 0xff);
	} else {
		int tcon = 256 - 1000000 / rate;
		write_dsp(CMD_RATE);
		write_dsp(tcon);
	}
}

void *sb_buffer(int *size)
{
	*size = 65536;
	return buffer;
}

void sb_start(int rate, int nchan)
{
	uint32_t addr;
	int size;

	/* for now just use the are after boot2. it's only used by the second stage
	 * loader and the VBE init code, none of which should overlap with audio playback.
	 * It's not necessary to use low memory. We can use up to the first 16mb for this,
	 * so if this becomes an issue, I'll make a DMA buffer allocator over 1mb.
	 * start the buffer from the next 64k boundary.
	 */
	addr = ((uint32_t)low_mem_buffer + 0xffff) & 0xffff0000;
	buffer = (void*)addr;

	xfer_mode = CMD_MODE_SIGNED;
	if(nchan > 1) {
		xfer_mode |= CMD_MODE_STEREO;
	}

	if(!(size = audio_callback(buffer, 65536))) {
		return;
	}

	interrupt(IRQ_TO_INTR(irq), intr_handler);

	write_dsp(CMD_ENABLE_OUTPUT);
	sb_set_output_rate(rate);
	start_dma_transfer(addr, size);
}

void sb_pause(void)
{
	write_dsp(CMD_PAUSE_DMA8);
}

void sb_continue(void)
{
	write_dsp(CMD_CONT_DMA8);
}

void sb_stop(void)
{
	write_dsp(CMD_END_DMA8);
	write_dsp(CMD_DISABLE_OUTPUT);
}

void sb_volume(int vol)
{
	/* TODO */
}

static void intr_handler()
{
	int size;

	/* ask for more data */
	if(!(size = audio_callback(buffer, 65536))) {
		sb_stop();
		return;
	}
	start_dma_transfer((uint32_t)buffer, size);

	/* acknowledge the interrupt */
	inb(REG_INTACK);
}

static void start_dma_transfer(uint32_t addr, int size)
{
	/* set up the next DMA transfer */
	dma_out(dma_chan, addr, size, DMA_SINGLE);

	/* program the DSP to accept the DMA transfer */
	write_dsp(CMD_START_DMA8);
	write_dsp(xfer_mode);
	size--;
	write_dsp(size & 0xff);
	write_dsp((size >> 8) & 0xff);
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

static void write_mix(unsigned char val, int reg)
{
	outb(reg, REG_MIXPORT);
	outb(val, REG_MIXDATA);
}

static unsigned char read_mix(int reg)
{
	outb(reg, REG_MIXPORT);
	return inb(REG_MIXDATA);
}

static int get_dsp_version(void)
{
	int major, minor;

	write_dsp(CMD_GET_VER);
	major = read_dsp();
	minor = read_dsp();

	return (major << 8) | minor;
}

static int sb16_detect_irq(void)
{
	int i, irqsel;
	static int irqtab[] = {2, 5, 7, 10};

	irq = 0;
	irqsel = read_mix(MIX_IRQ_SEL);
	for(i=0; i<4; i++) {
		if(irqsel & (1 << i)) {
			irq = irqtab[i];
			break;
		}
	}
	if(!irq) {
		/* try to force IRQ 5 */
		write_mix(2, MIX_IRQ_SEL);	/* bit1 selects irq 5 */

		/* re-read to verify */
		irqsel = read_mix(MIX_IRQ_SEL);
		if(irqsel != 2) {
			return -1;
		}
		irq = 5;
	}

	return irq;
}

static int sb16_detect_dma(void)
{
	int i, dmasel, tmp;
	static int dmatab[] = {0, 1, -1, 3, -1, 5, 6, 7};

	dma_chan = -1;
	dma16_chan = -1;
	dmasel = read_mix(MIX_DMA_SEL);
	for(i=0; i<4; i++) {
		if(dmasel & (1 << i)) {
			dma_chan = dmatab[i];
			break;
		}
	}
	for(i=5; i<8; i++) {
		if(dmasel & (1 << i)) {
			dma16_chan = dmatab[i];
			break;
		}
	}
	if(dma_chan == -1) {
		/* try to force DMA 1 */
		dmasel |= 2;
	}
	if(dma16_chan == -1) {
		/* try to force 16bit DMA 5 */
		dmasel |= 0x20;
	}

	if(dma_chan == -1 || dma16_chan == -1) {
		write_mix(dmasel, MIX_DMA_SEL);

		/* re-read to verify */
		tmp = read_mix(MIX_DMA_SEL);
		if(tmp != dmasel) {
			return -1;
		}
		dma_chan = 1;
		dma16_chan = 5;
	}

	return dma_chan;
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
