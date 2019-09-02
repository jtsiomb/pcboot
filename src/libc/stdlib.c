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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <assert.h>
#include <alloca.h>

int atoi(const char *str)
{
	return strtol(str, 0, 10);
}

long atol(const char *str)
{
	return strtol(str, 0, 10);
}

long strtol(const char *str, char **endp, int base)
{
	long acc = 0;
	int sign = 1;
	int valid = 0;
	const char *start = str;

	while(isspace(*str)) str++;

	if(base == 0) {
		if(str[0] == '0') {
			if(str[1] == 'x' || str[1] == 'X') {
				base = 16;
			} else {
				base = 8;
			}
		} else {
			base = 10;
		}
	}

	if(*str == '+') {
		str++;
	} else if(*str == '-') {
		sign = -1;
		str++;
	}

	while(*str) {
		long val = LONG_MAX;
		char c = tolower(*str);

		if(isdigit(c)) {
			val = *str - '0';
		} else if(c >= 'a' && c <= 'f') {
			val = 10 + c - 'a';
		} else {
			break;
		}
		if(val >= base) {
			break;
		}
		valid = 1;

		acc = acc * base + val;
		str++;
	}

	if(endp) {
		*endp = (char*)(valid ? str : start);
	}

	return sign > 0 ? acc : -acc;
}

void itoa(int val, char *buf, int base)
{
	static char rbuf[16];
	char *ptr = rbuf;
	int neg = 0;

	if(val < 0) {
		neg = 1;
		val = -val;
	}

	if(val == 0) {
		*ptr++ = '0';
	}

	while(val) {
		int digit = val % base;
		*ptr++ = digit < 10 ? (digit + '0') : (digit - 10 + 'a');
		val /= base;
	}

	if(neg) {
		*ptr++ = '-';
	}

	ptr--;

	while(ptr >= rbuf) {
		*buf++ = *ptr--;
	}
	*buf = 0;
}

void utoa(unsigned int val, char *buf, int base)
{
	static char rbuf[16];
	char *ptr = rbuf;

	if(val == 0) {
		*ptr++ = '0';
	}

	while(val) {
		unsigned int digit = val % base;
		*ptr++ = digit < 10 ? (digit + '0') : (digit - 10 + 'a');
		val /= base;
	}

	ptr--;

	while(ptr >= rbuf) {
		*buf++ = *ptr--;
	}
	*buf = 0;
}

double atof(const char *str)
{
	return strtod(str, 0);
}


double strtod(const char *str, char **endp)
{
	char *ep;
	const char *start = str;
	int valid = 0;
	long ival = 0, dval = 0;
	int ddig = 0;
	double res;

	/* integer part */
	ival = strtol(str, &ep, 10);
	if(ep == str && *str != '.') {
		if(endp) *endp = (char*)str;
		return 0.0;
	}
	if(ep != str) valid = 1;
	str = *ep == '.' ? ep + 1 : ep;
	if(!isdigit(*str)) {
		goto done;
	}
	valid = 1;

	dval = strtol(str, &ep, 10);
	assert(dval >= 0);
	ddig = ep - str;
	str = ep;

done:
	if(*endp) {
		*endp = (char*)(valid ? str : start);
	}

	res = (double)ival;
	if(ddig) {
		double d = (double)dval;
		while(ddig-- > 0) {
			d /= 10.0;
		}
		res += d;
	}
	return res;
}

int atexit(void (*func)(void))
{
	/* there's no concept of exiting at the moment, so this does nothing */
	return 0;
}

void abort(void)
{
	panic("Aborted\n");
}

#define QSORT_THRESHOLD	4
#define ITEM(idx)	((char*)arr + (idx) * itemsz)

#define SWAP(p, q) \
	do { \
		int nn = itemsz; \
		char *pp = (p); \
		char *qq = (q); \
		do { \
			char tmp = *pp; \
			*pp++ = *qq; \
			*qq++ = tmp; \
		} while(--nn > 0); \
	} while(0)

static void ins_sort(void *arr, size_t count, size_t itemsz, int (*cmp)(const void*, const void*))
{
	int i;
	char *it, *a, *b;

	if(count <= 1) return;

	it = (char*)arr + itemsz;
	for(i=1; i<count; i++) {
		a = it;
		it += itemsz;
		while(a > (char*)arr && cmp(a, (b = a - itemsz)) < 0) {
			SWAP(a, b);
			a -= itemsz;
		}
	}
}

void qsort(void *arr, size_t count, size_t itemsz, int (*cmp)(const void*, const void*))
{
	char *ma, *mb, *mc, *left, *right;
	size_t sepidx, nleft, nright;

	if(count <= 1) return;

	if(count < QSORT_THRESHOLD) {
		ins_sort(arr, count, itemsz, cmp);
		return;
	}

	ma = arr;
	mb = ITEM(count / 2);
	mc = ITEM(count - 1);
	if(cmp(ma, mb) < 0) SWAP(ma, mb);
	if(cmp(mc, ma) < 0) SWAP(mc, ma);

	left = ma + itemsz;
	right = mc - itemsz;
	for(;;) {
		while(cmp(left, ma) < 0) left += itemsz;
		while(cmp(ma, right) < 0) right -= itemsz;
		if(left >= right) break;
		SWAP(left, right);
	}
	SWAP(ma, right);
	sepidx = (right - (char*)arr) / itemsz;
	nleft = sepidx;
	nright = count - nleft - 1;

	qsort(ma, nleft, itemsz, cmp);
	qsort(right + itemsz, nright, itemsz, cmp);
}
