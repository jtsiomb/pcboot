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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "mem.h"
#include "panic.h"

struct mem_desc {
	size_t size;
	uint32_t magic;
	struct mem_desc *next;
};

#define MAGIC	0xdeadf00d

#define DESC_PTR(b)	((void*)((struct mem_desc*)(b) + 1))
#define PTR_DESC(p)	((struct mem_desc*)(p) - 1)

#define NUM_POOLS	16
#define FIRST_POOL_POW2	5
/* 2**(x+5) size pools: 0->32, 1->64, 2->128 .. 15->1048576 */
#define POOL_SIZE(x)	(1 << ((x) + FIRST_POOL_POW2))
#define MAX_POOL_SIZE	(1 << ((NUM_POOLS - 1) + FIRST_POOL_POW2))
#define MAX_POOL_PAGES	BYTES_TO_PAGES(MAX_POOL_SIZE)
static struct mem_desc *pools[NUM_POOLS];

static int add_to_pool(struct mem_desc *b);

static int pool_index(int sz)
{
	int x = 0;
	--sz;
	while(sz) {
		sz >>= 1;
		++x;
	}
	return x - FIRST_POOL_POW2;
}

void *malloc(size_t sz)
{
	int pidx, pg0;
	size_t total_sz, halfsz;
	struct mem_desc *mem, *other;

	total_sz = sz + sizeof(struct mem_desc);

	if(total_sz > MAX_POOL_SIZE) {
		/*printf("  allocation too big, hitting sys_alloc directly\n");*/
		if((pg0 = alloc_ppages(BYTES_TO_PAGES(total_sz))) == -1) {
			errno = ENOMEM;
			return 0;
		}
		mem = PAGE_TO_PTR(pg0);
		mem->magic = MAGIC;
		mem->size = total_sz;
		mem->next = 0;
		return DESC_PTR(mem);
	}

	pidx = pool_index(total_sz);
	while(pidx < NUM_POOLS) {
		if(pools[pidx]) {
			/* found a pool with a free block that fits */
			mem = pools[pidx];
			pools[pidx] = mem->next;
			mem->next = 0;

			/*printf("  using pool %d (size %ld)\n", pidx, (unsigned long)mem->size);*/

			/* while the mem size is larger than twice the allocation, split it */
			while(pidx-- > 0 && total_sz <= (halfsz = mem->size / 2)) {
				/*printf("  splitting %ld block in half and ", (unsigned long)mem->size);*/
				other = (struct mem_desc*)((char*)mem + halfsz);
				mem->size = other->size = halfsz;

				add_to_pool(other);
			}

			mem->magic = MAGIC;
			return DESC_PTR(mem);
		}

		++pidx;
	}

	/* did not find a free block, add a new one */
	pidx = NUM_POOLS - 1;
	if((pg0 = alloc_ppages(MAX_POOL_PAGES)) == -1) {
		errno = ENOMEM;
		return 0;
	}
	mem = PAGE_TO_PTR(pg0);
	mem->size = MAX_POOL_SIZE;
	mem->next = pools[pidx];
	mem->magic = MAGIC;
	pools[pidx] = mem;

	/* try again now that there is a free block */
	return malloc(sz);
}


void free(void *p)
{
	int pg0;
	struct mem_desc *mem = PTR_DESC(p);

	if(mem->magic != MAGIC) {
		panic("free: corrupted magic!\n");
	}

	if(mem->size > MAX_POOL_SIZE) {
		/*printf("foo_free(%p): %ld bytes. block too large for pools, returning to system\n",
				(void*)p, (unsigned long)mem->size);*/
		pg0 = ADDR_TO_PAGE(mem);
		free_ppages(pg0, BYTES_TO_PAGES(mem->size));
		return;
	}

	/*printf("foo_free(%p): block of %ld bytes and ", (void*)p, (unsigned long)mem->size);*/
	add_to_pool(mem);
}


void *calloc(size_t num, size_t size)
{
	void *ptr = malloc(num * size);
	if(ptr) {
		memset(ptr, 0, num * size);
	}
	return ptr;
}

void *realloc(void *ptr, size_t size)
{
	struct mem_desc *mem;
	void *newp;

	if(!ptr) {
		return malloc(size);
	}

	mem = PTR_DESC(ptr);
	if(mem->size >= size) {
		return ptr;	/* TODO: shrink */
	}

	if(!(newp = malloc(size))) {
		return 0;
	}
	free(ptr);
	return newp;
}


static int add_to_pool(struct mem_desc *mem)
{
	int pidx;
	struct mem_desc head;
	struct mem_desc *iter, *pnode;

	pidx = pool_index(mem->size);

	/*printf("adding %ld block to pool %d\n", (unsigned long)mem->size, pidx);*/

	iter = &head;
	head.next = pools[pidx];

	while(iter->next) {
		pnode = iter->next;
		if(mem->size == pnode->size) {	/* only coalesce same-sized blocks */
			if((char*)mem == (char*)pnode - pnode->size) {
				iter->next = pnode->next;	/* unlink pnode */
				pools[pidx] = head.next;
				mem->next = 0;
				mem->size += pnode->size;

				/*printf("  coalescing %p with %p and ", (void*)mem, (void*)pnode);*/
				return add_to_pool(mem);
			}
			if((char*)mem == (char*)pnode + pnode->size) {
				iter->next = pnode->next;	/* unlink pnode */
				pools[pidx] = head.next;
				pnode->next = 0;
				pnode->size += mem->size;

				/*printf("  coalescing %p with %p and ", (void*)mem, (void*)pnode);*/
				return add_to_pool(pnode);
			}
		}
		iter = iter->next;
	}

	/* otherwise just add it to the pool */
	mem->next = pools[pidx];
	pools[pidx] = mem;
	return 0;
}

