#include <string.h>
#include "video.h"
#include "asmops.h"
#include "keyb.h"

static uint16_t *framebuf;

int vbetest(void)
{
	if(!(framebuf = set_video_mode(640, 480, 16))) {
		return -1;
	}

	memset(framebuf, 0xff, 640 * 240 * 2);

	while(kb_getkey() == -1) {
		halt_cpu();
	}

	set_vga_mode(3);
	return 0;
}
