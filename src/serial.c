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
#include <string.h>
#include "serial.h"
#include "asmops.h"

#define UART1_BASE	0x3f8
#define UART2_BASE	0x2f8
#define UART1_IRQ	4
#define UART2_IRQ	3

#define UART_DATA	0
#define UART_INTR	1
#define UART_DIVLO	0
#define UART_DIVHI	1
#define UART_FIFO	2
#define UART_IID	2
#define UART_LCTL	3
#define UART_MCTL	4
#define UART_LSTAT	5
#define UART_MSTAT	6

/* interrupt enable register bits */
#define INTR_RECV	1
#define INTR_SEND	2
#define INTR_LSTAT	4
#define INTR_DELTA	8

/* fifo control register bits */
#define FIFO_ENABLE		0x01
#define FIFO_RECV_CLEAR	0x02
#define FIFO_SEND_CLEAR	0x04
#define FIFO_DMA		0x08
#define FIFO_TRIG_4		0x40
#define FIFO_TRIG_8		0x80
#define FIFO_TRIG_14	0xc0

/* interrupt id register bits */
#define IID_PENDING		0x01
#define IID_ID0			0x02
#define IID_ID1			0x04
#define IID_ID2			0x08
#define IID_FIFO_EN		0xc0

#define IID_SOURCE		0xe

#define IID_DELTA		0
#define IID_SEND		0x2
#define IID_RECV		0x4
#define IID_FIFO		0xc
#define IID_STATUS		0x6

/* line control register bits */
#define LCTL_BITS_8	0x03
#define LCTL_STOP_2	0x04
#define LCTL_DLAB	0x80
#define LCTL_8N1	LCTL_BITS_8
#define LCTL_8N2	(LCTL_BITS_8 | LCTL_STOP_2)

/* modem control register bits */
#define MCTL_DTR	0x01
#define MCTL_RTS	0x02
#define MCTL_OUT1	0x04
#define MCTL_OUT2	0x08
#define MCTL_LOOP	0x10

/* line status register bits */
#define LST_DRDY		0x01
#define LST_ERR_OVER	0x02
#define LST_ERR_PARITY	0x04
#define LST_ERR_FRAME	0x08
#define LST_ERR_BRK		0x10
#define LST_TREG_EMPTY	0x20
#define LST_TIDLE		0x40
#define LST_ERROR		0x80

/* modem status register bits */
#define MST_DELTA_CTS	0x01
#define MST_DELTA_DSR	0x02
#define MST_TERI		0x04
#define MST_DELTA_DCD	0x08
#define MST_CTS			0x10
#define MST_DSR			0x20
#define MST_RING		0x40
#define MST_DCD			0x80

/* interrupt controller stuff */
#define PIC1_CMD_PORT	0x20
#define PIC1_DATA_PORT	0x21
#define PIC2_CMD_PORT	0xa0
#define PIC2_DATA_PORT	0xa1
#define OCW2_EOI		0x20

#define COM_FMT_8N1		LCTL_8N1
#define COM_FMT_8N2		LCTL_8N2

struct serial_port {
	int base, intr;
	int blocking;

	char inbuf[256];
	int inbuf_ridx, inbuf_widx;
};

#define BNEXT(x)	(((x) + 1) & 0xff)
#define BEMPTY(b)	(b##_ridx == b##_widx)

/*
static int have_recv(int base);
static void recv_intr(void);*/

static struct serial_port ports[2];
static int num_open;

static int uart_base[] = {UART1_BASE, UART2_BASE};
/*static int uart_irq[] = {UART1_IRQ, UART2_IRQ};*/

int ser_open(int pidx, int baud, unsigned int mode)
{
	unsigned short div = 115200 / baud;
	int base; /*intr*/
	unsigned int fmt;
	/*int prev_if;*/

	if(pidx < 0 || pidx > 1) {
		printf("ser_open: invalid serial port: %d\n", pidx);
		return -1;
	}

	if(ports[pidx].base) {
		printf("ser_open: port %d already open!\n", pidx);
		return -1;
	}
	memset(ports + pidx, 0, sizeof ports[pidx]);

	base = uart_base[pidx];
	/*intr = uart_irq[pidx] | 8;*/

	if(mode & SER_8N2) {
		fmt = COM_FMT_8N2;
	} else {
		fmt = COM_FMT_8N1;
	}

	/*prev_if = disable_intr();*/
	/* TODO set interrupt handler */
	/* unmask the appropriate interrupt */
	/*outb(inb(PIC1_DATA_PORT) & ~(1 << uart_irq[pidx]), PIC1_DATA_PORT);*/

	outb(LCTL_DLAB, base + UART_LCTL);
	outb(div & 0xff, base + UART_DIVLO);
	outb((div >> 8) & 0xff, base + UART_DIVHI);
	outb(fmt, base + UART_LCTL);	/* fmt should be LCTL_8N1, LCTL_8N2 etc */
	outb(FIFO_ENABLE | FIFO_SEND_CLEAR | FIFO_RECV_CLEAR, base + UART_FIFO);
	outb(MCTL_DTR | MCTL_RTS | MCTL_OUT2, base + UART_MCTL);
	/*outb(INTR_RECV, base + UART_INTR);

	restore_intr(prev_if);*/

	ports[pidx].base = base;
	/*ports[pidx].intr = intr;*/
	ports[pidx].blocking = 1;
	++num_open;
	return pidx;
}

void ser_close(int fd)
{
	if(--num_open == 0) {
		/*int prev_if = disable_intr();*/
		/*outb(0, ports[fd].base + UART_INTR);*/
		outb(0, ports[fd].base + UART_MCTL);
		/*restore_intr(prev_if);*/
	}

	ports[fd].base = 0;
}

int ser_block(int fd)
{
	ports[fd].blocking = 1;
	return 0;
}

int ser_nonblock(int fd)
{
	ports[fd].blocking = 0;
	return 0;
}

int ser_pending(int fd)
{
	return !BEMPTY(ports[fd].inbuf);
}

/* if msec < 0: wait for ever */
int ser_wait(int fd, long msec)
{
	int res;
	while(!(res = ser_pending(fd))) {
		/* TODO timeout */
	}
	return res;
}

static int can_send(int fd)
{
	int base = ports[fd].base;
	return inb(base + UART_LSTAT) & LST_TREG_EMPTY;
}

void ser_putc(int fd, char c)
{
	int base = ports[fd].base;

	if(c == '\n') {
		ser_putc(fd, '\r');
	}

	while(!can_send(fd));
	/*while((inb(base + UART_MSTAT) & MST_CTS) == 0);*/
	outb(c, base + UART_DATA);
}

int ser_getc(int fd)
{
	struct serial_port *p = ports + fd;
	int have, c = -1;

	if(p->blocking) {
		while(!(have = ser_pending(fd)));
	} else {
		have = ser_pending(fd);
	}

	if(have) {
		c = p->inbuf[p->inbuf_ridx];
		p->inbuf_ridx = BNEXT(p->inbuf_ridx);
	}
	return c;
}

int ser_write(int fd, const char *buf, int count)
{
	int n = count;
	while(n--) {
		ser_putc(fd, *buf++);
	}
	return count;
}

int ser_read(int fd, char *buf, int count)
{
	int c, n = 0;
	while(n < count && (c = ser_getc(fd)) != -1) {
		*buf++ = c;
		++n;
	}
	return n;
}

char *ser_getline(int fd, char *buf, int bsz)
{
	static char linebuf[512];
	static int widx;
	int i, rd, size, offs;

	size = sizeof linebuf - widx;
	while(size && (rd = ser_read(fd, linebuf + widx, size)) > 0) {
		widx += rd;
		size -= rd;
	}

	linebuf[widx] = 0;

	for(i=0; i<widx; i++) {
		if(linebuf[i] == '\r' || linebuf[i] == '\n') {
			size = i >= bsz ? bsz - 1 : i;
			memcpy(buf, linebuf, size);
			buf[size] = 0;

			offs = i + 1;
			memmove(linebuf, linebuf + offs, widx - offs);
			widx -= offs;
			return buf;
		}
	}
	return 0;
}

#if 0
static int have_recv(int base)
{
	unsigned short stat = inb(base + UART_LSTAT);
	if(stat & LST_ERROR) {
		printf("serial receive error\n");
		panic();
	}
	return stat & LST_DRDY;
}

static void __interrupt __far recv_intr()
{
	int i, idreg, c;

	for(i=0; i<2; i++) {
		int base = uart_base[i];
		struct serial_port *p = ports + i;

		while(((idreg = inb(base + UART_IID)) & IID_PENDING) == 0) {
			while(have_recv(base)) {
				c = inb(base + UART_DATA);

				p->inbuf[p->inbuf_widx] = inb(base + UART_DATA);
				p->inbuf_widx = BNEXT(p->inbuf_widx);

				if(p->inbuf_widx == p->inbuf_ridx) {
					/* we overflowed, drop the oldest */
					p->inbuf_ridx = BNEXT(p->inbuf_ridx);
				}
			}
		}
	}

	outb(OCW2_EOI, PIC1_CMD_PORT);
}
#endif
