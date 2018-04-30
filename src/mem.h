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
#ifndef MEM_H_
#define MEM_H_

#define ADDR_TO_PAGE(x)			((uint32_t)(x) >> 12)
#define PAGE_TO_ADDR(x)			((uint32_t)(x) << 12)
#define PAGE_TO_PTR(x)			((void*)PAGE_TO_ADDR(x))

#define BYTES_TO_PAGES(x)		(((uint32_t)(x) + 4095) >> 12)

void init_mem(void);

int alloc_ppage(void);
void free_ppage(int pg);

/* allocate a number of consecutive pages */
int alloc_ppages(int count);
void free_ppages(int pg0, int count);

/* allocate a specific range of pages.
 * Fails (and returns -1) if any page in the requested range is not free.
 */
int alloc_ppage_range(int start, int size);
int free_ppage_range(int start, int size);

#endif	/* MEM_H_ */
