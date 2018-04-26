#include <stdio.h>
#include <ctype.h>
#include "segm.h"
#include "intr.h"
#include "keyb.h"
#include "timer.h"
#include "contty.h"

void set_mode13h(void);
void logohack(void);

void pcboot_main(void)
{
	init_segm();
	init_intr();
	kb_init();
	con_init();

	/* initialize the timer */
	init_timer();

	enable_intr();

	printf("PCBoot kernel initialized\n");

	for(;;) {
		int c;

		halt_cpu();
		while((c = kb_getkey()) >= 0) {
			if(c >= KB_F1 && c <= KB_F12) {
				set_mode13h();
				logohack();
			}
			if(isprint(c)) {
				printf("key: %d '%c'       \n", c, (char)c);
			} else {
				printf("key: %d            \n", c);
			}
		}
		if((nticks % 250) == 0) {
			printf("ticks: %ld\r", nticks);
		}
	}
}
