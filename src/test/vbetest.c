#include <stdio.h>
#include <string.h>
#include "video.h"
#include "asmops.h"
#include "keyb.h"
#include "psaux.h"
#include "contty.h"
#include "audio.h"

static void draw_cursor(int x, int y, uint16_t col);
static int click_sound_callback(void *buffer, int size, void *cls);

static uint16_t *framebuf;

#define CURSOR_XSZ	12
#define CURSOR_YSZ	16
static uint16_t cursor[] = {
	0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0xffff, 0x0001, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0xffff, 0x0001, 0x0001, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0xffff, 0x0001, 0x0001, 0x0001, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0xffff, 0x0001, 0x0001, 0x0001, 0x0001, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0xffff, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0xffff, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000,
	0xffff, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0xffff, 0x0000, 0x0000, 0x0000,
	0xffff, 0x0001, 0x0001, 0x0001, 0x0001, 0x0001, 0xffff, 0xffff, 0xffff, 0xffff, 0x0000, 0x0000,
	0xffff, 0x0001, 0x0001, 0x0001, 0x0001, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0xffff, 0x0001, 0x0001, 0xffff, 0x0001, 0x0001, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0xffff, 0x0001, 0xffff, 0xffff, 0x0001, 0x0001, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0xffff, 0xffff, 0x0000, 0x0000, 0xffff, 0x0001, 0x0001, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000,
	0xffff, 0x0000, 0x0000, 0x0000, 0xffff, 0x0001, 0x0001, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xffff, 0xffff, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

static int click;

/* defined in sndsamples.s */
extern signed char snd_click[];
extern int snd_click_size;

int vbetest(void)
{
	int i, j, nmodes, mx, my, idx;
	unsigned int st, prev_st = 0;
	struct video_mode vi;
	uint16_t *fbptr;

	nmodes = video_mode_count();
	printf("%d video modes found:\n", nmodes);
	for(i=0; i<nmodes; i++) {
		if(video_mode_info(i, &vi) == -1) {
			continue;
		}
		printf(" %04x: %dx%d %d bpp", vi.mode, vi.width, vi.height, vi.bpp);
		if(vi.bpp > 8) {
			printf(" (%d%d%d)\n", vi.rbits, vi.gbits, vi.bbits);
		} else {
			putchar('\n');
		}
	}

	if((idx = find_video_mode_idx(640, 480, 16)) == -1) {
		return -1;
	}
	video_mode_info(idx, &vi);

	if(!(framebuf = set_video_mode(vi.mode))) {
		return -1;
	}
	get_color_bits(&vi.rbits, &vi.gbits, &vi.bbits);
	get_color_shift(&vi.rshift, &vi.gshift, &vi.bshift);
	get_color_mask(&vi.rmask, &vi.gmask, &vi.bmask);

	fbptr = framebuf;
	for(i=0; i<480; i++) {
		for(j=0; j<640; j++) {
			int xor = i^j;
			uint16_t r = xor & 0xff;
			uint16_t g = (xor << 1) & 0xff;
			uint16_t b = (xor << 2) & 0xff;

			r >>= 8 - vi.rbits;
			g >>= 8 - vi.gbits;
			b >>= 8 - vi.bbits;

			*fbptr++ = ((r << vi.rshift) & vi.rmask) | ((g << vi.gshift) & vi.gmask) |
				((b << vi.bshift) & vi.bmask);
		}
	}

	set_mouse_bounds(0, 0, 639, 479);

	audio_set_callback(click_sound_callback, 0);

	/* empty the kb queue */
	while(kb_getkey() != -1);

	for(;;) {
		if(kb_getkey() != -1) {
			break;
		}

		st = mouse_state(&mx, &my);

		for(i=0; i<3; i++) {
			unsigned int bit = 1 << i;
			if(((st & bit) ^ (prev_st & bit)) & (st & bit)) {
				click = 1;
			}
		}
		if(click) {
			audio_play(22050, 1);
		}

		prev_st = st;

		draw_cursor(mx, my, st & 1 ? 0xf800 : (st & 2 ? 0x7e0 : (st & 4 ? 0x00ff : 0)));

		halt_cpu();
	}

	set_vga_mode(3);
	con_clear();
	return 0;
}

static void draw_cursor(int x, int y, uint16_t col)
{
	static uint16_t saved[CURSOR_XSZ * CURSOR_YSZ];
	static int saved_x = -1, saved_y, saved_w, saved_h;

	int i, j, w, h;
	uint16_t *dest, *src, *savp;

	if(saved_x >= 0) {
		dest = framebuf + saved_y * 640 + saved_x;
		src = saved;

		for(i=0; i<saved_h; i++) {
			for(j=0; j<saved_w; j++) {
				*dest++ = *src++;
			}
			src += CURSOR_XSZ - saved_w;
			dest += 640 - saved_w;
		}
	}

	dest = framebuf + y * 640 + x;
	src = cursor;
	savp = saved;

	w = 640 - x;
	if(w > CURSOR_XSZ) w = CURSOR_XSZ;
	h = 480 - y;
	if(h > CURSOR_YSZ) h = CURSOR_YSZ;

	saved_x = x;
	saved_y = y;
	saved_w = w;
	saved_h = h;

	for(i=0; i<h; i++) {
		for(j=0; j<w; j++) {
			uint16_t c = *src++;
			*savp++ = *dest;
			if(c) {
				if(c == 1) c = col;
				*dest = c;
			}
			dest++;
		}
		src += CURSOR_XSZ - w;
		dest += 640 - w;
		savp += CURSOR_XSZ - w;
	}
}

/* snd_click_size is < 65536 so we can just throw it all at once in there */
static int click_sound_callback(void *buffer, int size, void *cls)
{
	if(click) {
		memcpy(buffer, snd_click, snd_click_size);
		click = 0;
		return snd_click_size;
	}
	return 0;
}
