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


unsigned int color_mask(int nbits, int pos);

static struct vbe_info *vbe_info;
static uint16_t *modes;
static int mode_count;
static struct vbe_mode_info *mode_info;

void set_vga_mode(int mode)
{
	struct int86regs regs;

	memset(&regs, 0, sizeof regs);
	regs.eax = mode;
	int86(0x10, &regs);
}

static int init_once(void)
{
	int i;
	static int done_init;

	if(done_init) {
		return vbe_info && modes ? 0 : -1;
	}
	done_init = 1;

	/* check for VBE2 support and output some info */
	if(!(vbe_info = vbe_get_info())) {
		printf("VESA BIOS Extensions not available\n");
		return -1;
	}

	printf("VBE Version: %x.%x\n", vbe_info->version >> 8, vbe_info->version & 0xff);
	if(vbe_info->version < 0x200) {
		printf("VBE >=2.0 not available. VBE 1.x support not implemented yet.");
		return -1;
	}

	printf("Graphics adapter: %s, %s (%s)\n", (char*)VBEPTR(vbe_info->oem_vendor_name_ptr),
			(char*)VBEPTR(vbe_info->oem_product_name_ptr), (char*)VBEPTR(vbe_info->oem_product_rev_ptr));
	printf("Video memory: %dkb\n", vbe_info->total_mem << 6);

	modes = VBEPTR(vbe_info->vid_mode_ptr);

	mode_count = 0;
	for(i=0; i<1024; i++) {	/* impose an upper limit to avoid inf-loops */
		if(modes[i] == 0xffff) {
			break;	/* reached the end */
		}
		mode_count++;
	}
	return 0;
}

void *set_video_mode(int mode)
{
	if(init_once() == -1) return 0;
	if(mode < 0) return 0;

	if(!(mode_info = vbe_get_mode_info(mode))) {
		return 0;
	}

	if(vbe_set_mode(mode | VBE_MODE_LFB) == -1) {
		printf("Failed to set video mode %dx%d %dbpp\n", mode_info->xres, mode_info->yres, mode_info->bpp);
		return 0;
	}

	return (void*)mode_info->fb_addr;
}

int find_video_mode_idx(int xsz, int ysz, int bpp)
{
	int i, best = -1, best_bpp = 0;
	struct vbe_mode_info *inf;

	if(init_once() == -1) return -1;

	for(i=0; i<mode_count; i++) {
		inf = vbe_get_mode_info(modes[i] | VBE_MODE_LFB);
		if(!inf || inf->xres != xsz || inf->yres != ysz) {
			continue;
		}
		if((bpp <= 0 && inf->bpp > best_bpp) || SAME_BPP(inf->bpp, bpp)) {
			best = i;
			best_bpp = inf->bpp;
		}
	}

	if(best == -1) {
		printf("Requested video mode (%dx%d %dbpp) is unavailable\n", xsz, ysz, bpp);
		return -1;
	}
	return best;
}

int video_mode_count(void)
{
	if(init_once() == -1) return 0;
	return mode_count;
}

int video_mode_info(int n, struct video_mode *vid)
{
	struct vbe_mode_info *inf;

	if(init_once() == -1) return -1;

	if(!(inf = vbe_get_mode_info(modes[n] | VBE_MODE_LFB))) {
		return -1;
	}
	vid->mode = modes[n];
	vid->width = inf->xres;
	vid->height = inf->yres;
	vid->bpp = inf->bpp;
	vid->rbits = inf->rmask_size;
	vid->gbits = inf->gmask_size;
	vid->bbits = inf->bmask_size;
	vid->rshift = inf->rpos;
	vid->gshift = inf->gpos;
	vid->bshift = inf->bpos;
	vid->rmask = color_mask(inf->rmask_size, inf->rpos);
	vid->gmask = color_mask(inf->gmask_size, inf->gpos);
	vid->bmask = color_mask(inf->bmask_size, inf->bpos);
	return 0;
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
	if(!mode_info) {
		return -1;
	}
	*rmask = color_mask(mode_info->rmask_size, mode_info->rpos);
	*gmask = color_mask(mode_info->gmask_size, mode_info->gpos);
	*bmask = color_mask(mode_info->bmask_size, mode_info->bpos);
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

unsigned int color_mask(int nbits, int pos)
{
	static unsigned int maskbits[] = {0, 1, 3, 7, 0xf, 0x1f, 0x3f, 0x7f, 0xff};
	return maskbits[nbits] << pos;
}

const char *get_video_vendor(void)
{
	if(init_once() == -1) return 0;
	return (char*)VBEPTR(vbe_info->oem_vendor_name_ptr);
}

int get_video_mem_size(void)
{
	if(init_once() == -1) return 0;
	return vbe_info->total_mem << 6;
}
