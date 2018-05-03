#include <stdio.h>
#include <string.h>
#include "video.h"
#include "asmops.h"
#include "keyb.h"
#include "contty.h"

static uint16_t *framebuf;

int vbetest(void)
{
	int i, j, nmodes;
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

	if(!(framebuf = set_video_mode(find_video_mode(640, 480, 16)))) {
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

	while(kb_getkey() != -1);
	while(kb_getkey() == -1) {
		halt_cpu();
	}

	set_vga_mode(3);
	con_clear();
	return 0;
}
