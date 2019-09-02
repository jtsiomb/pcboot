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
#include <stdio.h>
#include <time.h>
#include <asmops.h>
#include "rtc.h"

/* CMOS I/O ports */
#define PORT_CTL	0x70
#define PORT_DATA	0x71

/* CMOS RTC registers */
#define REG_SEC			0
#define REG_ALARM_SEC	1
#define REG_MIN			2
#define REG_ALARM_MIN	3
#define REG_HOUR		4
#define REG_ALARM_HOUR	5
#define REG_WEEKDAY		6
#define REG_DAY			7
#define REG_MONTH		8
#define REG_YEAR		9
#define REG_STATA		10
#define REG_STATB		11
#define REG_STATC		12
#define REG_STATD		13

#define STATA_BUSY	(1 << 7)
#define STATB_24HR	(1 << 1)
#define STATB_BIN	(1 << 2)

#define HOUR_PM_BIT		(1 << 7)

#define BCD_TO_BIN(x)	((((x) >> 4) & 0xf) * 10 + ((x) & 0xf))

static void read_rtc(struct tm *tm);
static int read_reg(int reg);


void init_rtc(void)
{
	struct tm tm;

	read_rtc(&tm);
	start_time = mktime(&tm);

	printf("System real-time clock: %s", asctime(&tm));
}


static void read_rtc(struct tm *tm)
{
	int statb, pm;

	/* wait for any clock updates to finish */
	while(read_reg(REG_STATA) & STATA_BUSY);

	tm->tm_sec = read_reg(REG_SEC);
	tm->tm_min = read_reg(REG_MIN);
	tm->tm_hour = read_reg(REG_HOUR);
	tm->tm_mday = read_reg(REG_DAY);
	tm->tm_mon = read_reg(REG_MONTH);
	tm->tm_year = read_reg(REG_YEAR);

	/* in 12hour mode, bit 7 means post-meridiem */
	pm = tm->tm_hour & HOUR_PM_BIT;
	tm->tm_hour &= ~HOUR_PM_BIT;

	/* convert to binary if needed */
	statb = read_reg(REG_STATB);
	if(!(statb & STATB_BIN)) {
		tm->tm_sec = BCD_TO_BIN(tm->tm_sec);
		tm->tm_min = BCD_TO_BIN(tm->tm_min);
		tm->tm_hour = BCD_TO_BIN(tm->tm_hour);
		tm->tm_mday = BCD_TO_BIN(tm->tm_mday);
		tm->tm_mon = BCD_TO_BIN(tm->tm_mon);
		tm->tm_year = BCD_TO_BIN(tm->tm_year);
	}

	/* make the year an offset from 1900 */
	if(tm->tm_year < 100) {
		tm->tm_year += 100;
	} else {
		tm->tm_year -= 1900;
	}

	/* if tm_hour is in 12h mode, convert to 24h */
	if(!(statb & STATB_24HR)) {
		if(tm->tm_hour == 12) {
			tm->tm_hour = 0;
		}
		if(pm) {
			tm->tm_hour += 12;
		}
	}

	tm->tm_mon -= 1;	/* we want months to start from 0 */
}

static int read_reg(int reg)
{
	unsigned char val;
	outb(reg, PORT_CTL);
	iodelay();
	val = inb(PORT_DATA);
	iodelay();
	return val;
}
