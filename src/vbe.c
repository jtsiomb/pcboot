#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "vbe.h"
#include "asmops.h"
#include "int86.h"

#define SEG_ADDR(s)	((uint32_t)(s) << 4)

#define MODE_LFB	(1 << 14)

extern unsigned char low_mem_buffer[];

struct vbe_info *vbe_get_info(void)
{
	struct vbe_info *info;
	struct int86regs regs;

	info = (struct vbe_info*)low_mem_buffer;

	memcpy(info->sig, "VBE2", 4);

	memset(&regs, 0, sizeof regs);
	regs.es = (uint32_t)info >> 4;
	regs.eax = 0x4f00;
	int86(0x10, &regs);

	if((regs.eax & 0xffff) != 0x4f) {
		return 0;
	}

	return info;
}

struct vbe_mode_info *vbe_get_mode_info(int mode)
{
	struct vbe_mode_info *mi;
	struct int86regs regs;

	mi = (struct vbe_mode_info*)(low_mem_buffer + 512);

	memset(&regs, 0, sizeof regs);
	regs.es = (uint32_t)mi >> 4;
	regs.eax = 0x4f01;
	regs.ecx = mode;
	int86(0x10, &regs);

	if((regs.eax & 0xffff) != 0x4f) {
		return 0;
	}

	return mi;
}

int vbe_set_mode(int mode)
{
	struct int86regs regs;

	memset(&regs, 0, sizeof regs);
	regs.eax = 0x4f02;
	regs.ebx = mode;
	int86(0x10, &regs);

	if(regs.eax == 0x100) {
		return -1;
	}
	return 0;
}

void print_mode_info(struct vbe_mode_info *mi)
{
	static unsigned int maskbits[] = {0, 1, 3, 7, 0xf, 0x1f, 0x3f, 0x7f, 0xff};

	printf("resolution: %dx%d\n", mi->xres, mi->yres);
	printf("color depth: %d\n", mi->bpp);
	printf("mode attributes: %x\n", mi->mode_attr);
	printf("bytes per scanline: %d\n", mi->scanline_bytes);
	printf("number of planes: %d\n", (int)mi->num_planes);
	printf("number of banks: %d\n", (int)mi->num_banks);
	printf("mem model: %d\n", (int)mi->mem_model);
	printf("red bits: %d (mask: %x)\n", (int)mi->rmask_size, maskbits[mi->rmask_size] << mi->rpos);
	printf("green bits: %d (mask: %x)\n", (int)mi->gmask_size, maskbits[mi->gmask_size] << mi->gpos);
	printf("blue bits: %d (mask: %x)\n", (int)mi->bmask_size, maskbits[mi->bmask_size] << mi->bpos);
	printf("framebuffer address: %x\n", (unsigned int)mi->fb_addr);
}
