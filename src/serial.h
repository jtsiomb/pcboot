#ifndef SERIAL_H_
#define SERIAL_H_

#define SER_8N1		0
#define SER_8N2		1
#define SER_HWFLOW	2

int ser_open(int pidx, int baud, unsigned int mode);
void ser_close(int fd);

int ser_block(int fd);
int ser_nonblock(int fd);

int ser_pending(int fd);
/* if msec < 0: wait for ever */
int ser_wait(int fd, long msec);

void ser_putc(int fd, char c);
int ser_getc(int fd);

int ser_write(int fd, const char *buf, int count);
int ser_read(int fd, char *buf, int count);

#define ser_putchar(c)	ser_putc(0, c)

char *ser_getline(int fd, char *buf, int bsz);


#endif	/* SERIAL_H_ */
