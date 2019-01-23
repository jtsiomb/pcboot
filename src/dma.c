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
#include "dma.h"
#include "asmops.h"

/* 8bit DMA ports */
#define DMA_0_ADDR	0x00
#define DMA_0_COUNT	0x01
#define DMA_1_ADDR	0x02
#define DMA_1_COUNT	0x03
#define DMA_2_ADDR	0x04
#define DMA_2_COUNT	0x05
#define DMA_3_ADDR	0x06
#define DMA_3_COUNT	0x07
/* 16bit DMA ports */
#define DMA_4_ADDR	0xc0
#define DMA_4_COUNT	0xc2
#define DMA_5_ADDR	0xc4
#define DMA_5_COUNT	0xc6
#define DMA_6_ADDR	0xc8
#define DMA_6_COUNT	0xca
#define DMA_7_ADDR	0xcc
#define DMA_7_COUNT	0xce

#define DMA_ADDR(c)	\
	((c < 4) ? DMA_0_ADDR + ((c) << 1) : (DMA_4_ADDR + ((c) << 2)))
#define DMA_COUNT(c) \
	((c < 4) ? DMA_0_COUNT + ((c) << 1) : (DMA_4_COUNT + ((c) << 2)))

#define DMA8_MASK			0x0a
#define DMA8_MODE			0x0b
#define DMA8_CLR_FLIPFLOP	0x0c
#define DMA8_RESET			0x0d
#define DMA8_MASK_RST		0x0e
#define DMA8_RMASK			0x0f
#define DMA16_MASK			0xd4
#define DMA16_MODE			0xd6
#define DMA16_CLR_FLIPFLOP	0xd8
#define DMA16_RESET			0xda
#define DMA16_MASK_RST		0xdc
#define DMA16_RMASK		0xde

#define DMA_MASK(c)	((c) < 4 ? DMA8_MASK : DMA16_MASK)
#define DMA_MODE(c)	((c) < 4 ? DMA8_MODE : DMA16_MODE)
#define DMA_CLR_FLIPFLOP(c)	((c) < 4 ? DMA8_CLR_FLIPFLOP : DMA16_CLR_FLIPFLOP)
#define DMA_RESET(c)	((c) < 4 ? DMA8_RESET : DMA16_RESET)
#define DMA_MASK_RST(c)	((c) < 4 ? DMA8_MASK_RST : DMA16_MASK_RST)
#define DMA_RMASK(c)	((c) < 4 ? DMA8_RMASK : DMA16_RMASK)

#define DMA_0_PAGE		0x87
#define DMA_1_PAGE		0x83
#define DMA_2_PAGE		0x81
#define DMA_3_PAGE		0x82
#define DMA_4_PAGE		0x8f
#define DMA_5_PAGE		0x8b
#define DMA_6_PAGE		0x89
#define DMA_7_PAGE		0x8a

#define MODE_CHAN(x)	((x) & 3)
#define MODE_WRITE		0x04
#define MODE_READ		0x08
#define MODE_AUTO		0x10
#define MODE_DECR		0x20
#define MODE_SINGLE		0x40
#define MODE_BLOCK		0x80
#define MODE_CASCADE	0xc0

#define MASK_CHAN(x)	((x) & 3)
#define MASK_DISABLE	0x04

#define RMASK_CHAN(x)	(1 << ((x) & 3))

#define IS_16BIT(c)	((c) >= 4)

static void dma_io(int chan, uint32_t phyaddr, int size, unsigned int flags, unsigned int dir);
static inline void mask(int chan);
static inline void unmask(int chan);

static int page_port[] = {
	DMA_0_PAGE, DMA_1_PAGE, DMA_2_PAGE, DMA_3_PAGE,
	DMA_4_PAGE, DMA_5_PAGE, DMA_6_PAGE, DMA_7_PAGE
};

void dma_out(int chan, uint32_t phyaddr, int size, unsigned int flags)
{
	dma_io(chan, phyaddr, size, flags, MODE_READ);
}

void dma_in(int chan, uint32_t phyaddr, int size, unsigned int flags)
{
	dma_io(chan, phyaddr, size, flags, MODE_WRITE);
}

static void dma_io(int chan, uint32_t phyaddr, int size, unsigned int flags, unsigned int dir)
{
	unsigned int mode;
	int addr_port, count_port;

	addr_port = DMA_ADDR(chan);
	count_port = DMA_COUNT(chan);

	mask(chan);
	outb(0, DMA_CLR_FLIPFLOP(chan));

	/* first 2 bits of flags correspond to the mode bits 6,7 */
	mode = ((flags & 3) << 6) | dir | MODE_CHAN(chan);
	if(flags & DMA_DECR) mode |= MODE_DECR;
	if(flags & DMA_AUTO) mode |= MODE_AUTO;
	outb(mode, DMA_MODE(chan));

	if(IS_16BIT(chan)) {
		phyaddr >>= 1;
		size >>= 1;
	}

	outb(phyaddr & 0xff, addr_port);
	outb((phyaddr >> 8) & 0xff, addr_port);
	outb((phyaddr >> 16) & 0xff, page_port[chan]);

	size--;
	outb(size & 0xff, count_port);
	outb((size >> 8) & 0xff, count_port);

	unmask(chan);
}

static inline void mask(int chan)
{
	outb(MASK_CHAN(chan) | MASK_DISABLE, DMA_MASK(chan));
}

static inline void unmask(int chan)
{
	outb(MASK_CHAN(chan), DMA_MASK(chan));
}
