#include <stdio.h>
#include "intr.h"
#include "asmops.h"
#include "timer.h"
#include "config.h"

/* frequency of the oscillator driving the 8254 timer */
#define OSC_FREQ_HZ		1193182

/* macro to divide and round to the nearest integer */
#define DIV_ROUND(a, b) ((a) / (b) + ((a) % (b)) / ((b) / 2))

/* I/O ports connected to the 8254 */
#define PORT_DATA0	0x40
#define PORT_DATA1	0x41
#define PORT_DATA2	0x42
#define PORT_CMD	0x43

/* command bits */
#define CMD_CHAN0			0
#define CMD_CHAN1			(1 << 6)
#define CMD_CHAN2			(2 << 6)
#define CMD_RDBACK			(3 << 6)

#define CMD_LATCH			0
#define CMD_ACCESS_LOW		(1 << 4)
#define CMD_ACCESS_HIGH		(2 << 4)
#define CMD_ACCESS_BOTH		(3 << 4)

#define CMD_OP_INT_TERM		0
#define CMD_OP_ONESHOT		(1 << 1)
#define CMD_OP_RATE			(2 << 1)
#define CMD_OP_SQWAVE		(3 << 1)
#define CMD_OP_SOFT_STROBE	(4 << 1)
#define CMD_OP_HW_STROBE	(5 << 1)

#define CMD_MODE_BIN		0
#define CMD_MODE_BCD		1


#define MSEC_TO_TICKS(ms)	((ms) * TICK_FREQ_HZ / 1000)

struct timer_event {
	int dt;	/* remaining ticks delta from the previous event */
	struct timer_event *next;
};

/* defined in intr_asm.S */
void intr_entry_fast_timer(void);

static struct timer_event *evlist;


void init_timer(void)
{
	/* calculate the reload count: round(osc / freq) */
	int reload_count = DIV_ROUND(OSC_FREQ_HZ, TICK_FREQ_HZ);

	/* set the mode to square wave for channel 0, both low
	 * and high reload count bytes will follow...
	 */
	outb(CMD_CHAN0 | CMD_ACCESS_BOTH | CMD_OP_SQWAVE, PORT_CMD);

	/* write the low and high bytes of the reload count to the
	 * port for channel 0
	 */
	outb(reload_count & 0xff, PORT_DATA0);
	outb((reload_count >> 8) & 0xff, PORT_DATA0);

	/* set the timer interrupt handler */
	/*interrupt(IRQ_TO_INTR(0), timer_handler);*/
	/* set low level fast timer interrupt routine directly in the IDT */
	set_intr_entry(IRQ_TO_INTR(0), intr_entry_fast_timer);
}

/*
static void timer_handler(int inum)
{
	nticks++;
}
*/
