#ifndef VBE_H_
#define VBE_H_

#include <inttypes.h>

#define VBE_ATTR_LFB	(1 << 7)
#define VBE_MODE_LFB	(1 << 14)

struct vbe_info {
	uint8_t sig[4];
	uint16_t version;
	uint32_t oem_str_ptr;
	uint8_t caps[4];			/* capabilities */
	uint32_t vid_mode_ptr;		/* vbefarptr to video mode list */
	uint16_t total_mem;			/* num of 64k mem blocks */
	uint16_t oem_sw_rev;		/* VBE implementation software revision */
	uint32_t oem_vendor_name_ptr;
	uint32_t oem_product_name_ptr;
	uint32_t oem_product_rev_ptr;
	uint8_t reserved[222];
	uint8_t oem_data[256];
} __attribute__((packed));

struct vbe_mode_info {
	uint16_t mode_attr;
	uint8_t wina_attr, winb_attr;
	uint16_t win_gran, win_size;
	uint16_t wina_seg, winb_seg;
	uint32_t win_func;
	uint16_t scanline_bytes;

	/* VBE 1.2 and above */
	uint16_t xres, yres;
	uint8_t xcharsz, ycharsz;
	uint8_t num_planes;
	uint8_t bpp;
	uint8_t num_banks;
	uint8_t mem_model;
	uint8_t bank_size;		/* bank size in KB */
	uint8_t num_img_pages;
	uint8_t reserved1;

	/* direct color fields */
	uint8_t rmask_size, rpos;
	uint8_t gmask_size, gpos;
	uint8_t bmask_size, bpos;
	uint8_t xmask_size, xpos;
	uint8_t cmode_info;		/* direct color mode attributes */

	/* VBE 2.0 and above */
	uint32_t fb_addr;		/* physical address of the linear framebuffer */
	uint32_t reserved2;
	uint16_t reserved3;

	uint8_t reserved4[206];
} __attribute__((packed));

struct vbe_info *vbe_get_info(void);
struct vbe_mode_info *vbe_get_mode_info(int mode);

int vbe_set_mode(int mode);

void print_mode_info(struct vbe_mode_info *modei);

#endif	/* VBE_H_ */
