#ifndef CONTTY_H_
#define CONTTY_H_

int con_init(void);
void con_show_cursor(int show);
void con_cursor(int x, int y);
void con_fgcolor(int c);
void con_bgcolor(int c);
void con_clear(void);
void con_putchar(int c);

#endif	/* CONTTY_H_ */
