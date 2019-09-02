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
/* random number generator, based on this description of the algorithm
 * used by the GNU libc: https://www.mathstat.dal.ca/~selinger/random
 */
#include <stdlib.h>
#include <inttypes.h>

static int init_done;
static int32_t rng[34];
static int32_t *ptr0, *ptr1;

int rand(void)
{
	int res;

	if(!init_done) {
		srand(1);
	}

	*ptr1 += *ptr0;
	res = (uint32_t)*ptr1 >> 1;
	if(++ptr0 >= rng + 34) ptr0 = rng;
	if(++ptr1 >= rng + 34) ptr1 = rng;

	return res;
}

void srand(unsigned int seed)
{
	int i;

	init_done = 1;
	if(seed == 0) seed = 1;

	rng[0] = seed;
	for(i=1; i<31; i++) {
		rng[i] = (16807 * rng[i - 1]) % RAND_MAX;
		if(rng[i] < 0) rng[i] += RAND_MAX;
	}
	for(i=31; i<34; i++) {
		rng[i] = rng[i - 31];
	}
	ptr0 = rng + 3;
	ptr1 = rng + 31;

	for(i=34; i<344; i++) {
		rand();
	}
}
