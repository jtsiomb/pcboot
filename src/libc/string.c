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
#include <string.h>

/*
void memset(void *s, int c, size_t n)
{
	char *ptr = s;
	while(n--) {
		*ptr++ = c;
	}
}
*/

/* Does the same thing as memset only with 16bit values.
 * n in this case is the number of values, not the number of bytes.
 */
/*
void memset16(void *s, int c, size_t n)
{
	int16_t *ptr = s;
	while(n--) {
		*ptr++ = c;
	}
}
*/
/*
void *memcpy(void *dest, const void *src, size_t n)
{
	char *dptr = dest;
	const char *sptr = src;

	while(n--) {
		*dptr++ = *sptr++;
	}
	return dest;
}
*/

void *memmove(void *dest, const void *src, size_t n)
{
	int i;
	char *dptr;
	const char *sptr;

	if(dest <= src) {
		/* forward copy */
		dptr = dest;
		sptr = src;
		for(i=0; i<n; i++) {
			*dptr++ = *sptr++;
		}
	} else {
		/* backwards copy */
		dptr = (char*)dest + n - 1;
		sptr = (const char*)src + n - 1;
		for(i=0; i<n; i++) {
			*dptr-- = *sptr--;
		}
	}

	return dest;
}

size_t strlen(const char *s)
{
	size_t len = 0;
	while(*s++) len++;
	return len;
}

char *strchr(const char *s, int c)
{
	while(*s) {
		if(*s == c) {
			return (char*)s;
		}
		s++;
	}
	return 0;
}

char *strrchr(const char *s, int c)
{
	const char *ptr = s;

	/* find the end */
	while(*ptr) ptr++;

	/* go back checking for c */
	while(--ptr >= s) {
		if(*ptr == c) {
			return (char*)ptr;
		}
	}
	return 0;
}

char *strstr(const char *str, const char *substr)
{
	while(*str) {
		const char *s1 = str;
		const char *s2 = substr;

		while(*s1 && *s1 == *s2) {
			s1++;
			s2++;
		}
		if(!*s2) {
			return (char*)str;
		}
		str++;
	}
	return 0;
}

int strcmp(const char *s1, const char *s2)
{
	while(*s1 && *s1 == *s2) {
		s1++;
		s2++;
	}
	return *s1 - *s2;
}
