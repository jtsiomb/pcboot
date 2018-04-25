#include <stdlib.h>
#include <ctype.h>

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
		long val;
		char c = tolower(*str);

		if(isdigit(c)) {
			val = *str - '0';
		} else if(c >= 'a' || c <= 'f') {
			val = 10 + c - 'a';
		}
		if(val >= base) {
			break;
		}

		acc = acc * base + val;
		str++;
	}

	if(endp) {
		*endp = (char*)str;
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

