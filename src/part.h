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
#ifndef PART_H_
#define PART_H_

#include <inttypes.h>

#define PART_ACT_BIT	(1 << 9)
#define PART_PRIM_BIT	(1 << 10)

#define PART_TYPE(attr)		((attr) & 0xff)
#define PART_IS_ACT(attr)	((attr) & PART_ACT_BIT)
#define PART_IS_PRIM(attr)	((attr) & PART_PRIM_BIT)

struct partition {
	uint32_t start_sect;
	uint32_t size_sect;

	unsigned int attr;
};

int read_partitions(int dev, struct partition *ptab, int ptabsz);
void print_partition_table(struct partition *ptab, int npart);

#endif	/* PART_H_ */
