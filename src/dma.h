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
#ifndef DMA_H_
#define DMA_H_

#include <inttypes.h>

enum {
	DMA_SINGLE	= 0x01,
	DMA_BLOCK	= 0x02,
	DMA_CASCADE = DMA_SINGLE | DMA_BLOCK,
	DMA_DECR	= 0x08,
	DMA_AUTO	= 0x10
};

void dma_out(int chan, uint32_t phyaddr, int size, unsigned int flags);
void dma_in(int chan, uint32_t phyaddr, int size, unsigned int flags);

#endif	/* DMA_H_ */
