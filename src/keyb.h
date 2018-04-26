#ifndef KEYB_H_
#define KEYB_H_

#define KB_ANY		(-1)
#define KB_ALT		(-2)
#define KB_CTRL		(-3)
#define KB_SHIFT	(-4)

/* special keys */
enum {
	KB_ESC = 27,
	KB_INSERT, KB_DEL, KB_HOME, KB_END, KB_PGUP, KB_PGDN,
	KB_LEFT, KB_RIGHT, KB_UP, KB_DOWN,
	KB_NUM_DOT, KB_NUM_ENTER, KB_NUM_PLUS, KB_NUM_MINUS, KB_NUM_MUL, KB_NUM_DIV,
	KB_NUM_0, KB_NUM_1, KB_NUM_2, KB_NUM_3, KB_NUM_4,
	KB_NUM_5, KB_NUM_6, KB_NUM_7, KB_NUM_8, KB_NUM_9,
	KB_BACKSP = 127,

	KB_LALT, KB_RALT,
	KB_LCTRL, KB_RCTRL,
	KB_LSHIFT, KB_RSHIFT,
	KB_F1, KB_F2, KB_F3, KB_F4, KB_F5, KB_F6,
	KB_F7, KB_F8, KB_F9, KB_F10, KB_F11, KB_F12,
	KB_CAPSLK, KB_NUMLK, KB_SCRLK, KB_SYSRQ
};

void kb_init(void);

/* Boolean predicate for testing the current state of a particular key.
 * You may also pass KB_ANY to test if any key is held down.
 */
int kb_isdown(int key);

/* waits for any keypress */
void kb_wait(void);

/* removes and returns a single key from the input buffer. */
int kb_getkey(void);

void kb_putback(int key);

#endif	/* KEYB_H_ */
