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
#include "video.h"
#include "vbe.h"
#include "int86.h"

#define REALPTR(s, o)	(void*)(((uint32_t)(s) << 4) + (uint32_t)(o))
#define VBEPTR(x)		REALPTR(((x) & 0xffff0000) >> 16, (x) & 0xffff)
#define VBEPTR_SEG(x)	(((x) & 0xffff0000) >> 16)
#define VBEPTR_OFF(x)	((x) & 0xffff)


#define SAME_BPP(a, b)	\
	((a) == (b) || ((a) == 16 && (b) == 15) || ((a) == 15 && (b) == 16) || \
	 ((a) == 32 && (b) == 24) || ((a) == 24 && (b) == 32))



static struct vbe_info *vbe_info;
static struct vbe_mode_info *mode_info;

void set_vga_mode(int mode)
{
	struct int86regs regs;

	memset(&regs, 0, sizeof regs);
	regs.eax = mode;
	int86(0x10, &regs);
}

void *set_video_mode(int xsz, int ysz, int bpp)
{
	int i;
	static uint16_t *modes;
	uint16_t best = 0;

	/* check for VBE2 support and output some info */
	if(!vbe_info) {
		if(!(vbe_info = vbe_get_info())) {
			printf("VESA BIOS Extensions not available\n");
			return 0;
		}

		printf("VBE Version: %x.%x\n", vbe_info->version >> 8, vbe_info->version & 0xff);
		if(vbe_info->version < 0x200) {
			printf("VBE >=2.0 not available. VBE 1.x support not implemented yet.");
			return 0;
		}

		printf("Graphics adapter: %s, %s (%s)\n", (char*)VBEPTR(vbe_info->oem_vendor_name_ptr),
				(char*)VBEPTR(vbe_info->oem_product_name_ptr), (char*)VBEPTR(vbe_info->oem_product_rev_ptr));
		printf("Video memory: %dkb\n", vbe_info->total_mem << 6);

		modes = VBEPTR(vbe_info->vid_mode_ptr);
	}

	for(i=0; i<1024; i++) {	/* impose an upper limit to avoid inf-loops */
		if(modes[i] == 0xffff) {
			break;	/* reached the end */
		}

		mode_info = vbe_get_mode_info(modes[i] | VBE_MODE_LFB);
		if(!mode_info || mode_info->xres != xsz || mode_info->yres != ysz) {
			continue;
		}
		if(SAME_BPP(mode_info->bpp, bpp)) {
			best = modes[i];
		}
	}

	if(best) {
		mode_info = vbe_get_mode_info(best);
	} else {
		printf("Requested video mode (%dx%d %dbpp) is unavailable\n", xsz, ysz, bpp);
		return 0;
	}

	if(vbe_set_mode(best | VBE_MODE_LFB) == -1) {
		printf("Failed to set video mode %dx%d %dbpp\n", mode_info->xres, mode_info->yres, mode_info->bpp);
		return 0;
	}

	return (void*)mode_info->fb_addr;
}

int get_color_bits(int *rbits, int *gbits, int *bbits)
{
	if(!mode_info) {
		return -1;
	}
	*rbits = mode_info->rmask_size;
	*gbits = mode_info->gmask_size;
	*bbits = mode_info->bmask_size;
	return 0;
}

int get_color_mask(unsigned int *rmask, unsigned int *gmask, unsigned int *bmask)
{
	static unsigned int maskbits[] = {0, 1, 3, 7, 0xf, 0x1f, 0x3f, 0x7f, 0xff};
	if(!mode_info) {
		return -1;
	}
	*rmask = maskbits[mode_info->rmask_size] << mode_info->rpos;
	*gmask = maskbits[mode_info->gmask_size] << mode_info->gpos;
	*bmask = maskbits[mode_info->bmask_size] << mode_info->bpos;
	return 0;
}

int get_color_shift(int *rshift, int *gshift, int *bshift)
{
	if(!mode_info) {
		return -1;
	}
	*rshift = mode_info->rpos;
	*gshift = mode_info->gpos;
	*bshift = mode_info->bpos;
	return 0;
}

