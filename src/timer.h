/*
256boss - bootable launcher for 256byte intros
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
#ifndef _TIMER_H_
#define _TIMER_H_

#include "config.h"

#define MSEC_TO_TICKS(ms)	((ms) * TICK_FREQ_HZ / 1000)
#define TICKS_TO_MSEC(tk)	((tk) * 1000 / TICK_FREQ_HZ)

volatile unsigned long nticks;

void init_timer(void);

/*
int sys_sleep(int sec);
void sleep(unsigned long msec);
*/

/* schedule a function to be called 'msec' milliseconds into the future
 * warning: this function will be called directly from the timer interrupt, and
 * must be extremely quick.
 */
void set_alarm(unsigned long msec, void (*func)(void));
void cancel_alarm(void (*func)(void));

#endif	/* _TIMER_H_ */
