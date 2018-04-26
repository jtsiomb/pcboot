#ifndef _TIMER_H_
#define _TIMER_H_

unsigned long nticks;

void init_timer(void);

int sys_sleep(int sec);
void sleep(unsigned long msec);

#endif	/* _TIMER_H_ */
