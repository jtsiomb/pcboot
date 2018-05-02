#include <stdio.h>
#include <string.h>
#include "video.h"
#include "asmops.h"
#include "keyb.h"

static uint16_t *framebuf;

int vbetest(void)
{
	int i, nmodes;
	struct video_mode vi;

	nmodes = video_mode_count();
	printf("%d video modes found:\n", nmodes);
	for(i=0; i<nmodes; i++) {
		if(video_mode_info(i, &vi) == -1) {
			continue;
		}
		printf(" %04x: %dx%d %d bpp (%d%d%d)\n", vi.mode, vi.width, vi.height, vi.bpp,
				vi.rbits, vi.gbits, vi.bbits);
	}

	if(!(framebuf = set_video_mode(find_video_mode(640, 480, 16)))) {
		return -1;
	}

	memset(framebuf, 0xff, 640 * 240 * 2);

	while(kb_getkey() == -1) {
		halt_cpu();
	}

	set_vga_mode(3);
	return 0;
}
