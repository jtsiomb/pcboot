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

struct vbe_edid_chroma {
	unsigned char redgreen_xy_lsb;
	unsigned char bluewhite_xy_lsb;
	unsigned char redx_msb, redy_msb;
	unsigned char greenx_msb, greeny_msb;
	unsigned char bluex_msb, bluey_msb;
	unsigned char whitex_msb, whitey_msb;
} __attribute__((packed));

struct vbe_edid_timing {
	uint16_t dotclock;
	unsigned char hactive_lsb, hblank_lsb, hact_hblank_msb;
	unsigned char vactive_lsb, vblank_lsb, vact_vblank_msb;
	unsigned char hporch_lsb, hsync_lsb;
	unsigned char vporch_vsync_lsb;
	unsigned char hvporch_hvsync_msb;
	unsigned char hsize_lsb, vsize_lsb;	/* mm */
	unsigned char hsize_vsize_msb;
	unsigned char hborder, vborder;
	unsigned char features;
} __attribute__((packed));


#define VBE_EDID_MAGIC	"\0\xff\xff\xff\xff\xff\xff\0"

struct vbe_edid {
	char magic[8];
	uint16_t vendor;
	uint16_t product;
	uint32_t serial;
	unsigned char week, year;
	unsigned char ver_major, ver_minor;

	unsigned char vidinp;
	unsigned char hsize, vsize;
	unsigned char gamma;
	unsigned char features;

	struct vbe_edid_chroma chroma;

	uint16_t modes_std;
	unsigned char modes_ext;
	uint16_t timing_std;

	struct vbe_edid_timing timing[4];
	unsigned char num_ext, csum;

} __attribute__((packed));

struct vbe_info *vbe_get_info(void);
struct vbe_mode_info *vbe_get_mode_info(int mode);

int vbe_set_mode(int mode);

void print_mode_info(struct vbe_mode_info *modei);

int vbe_get_edid(struct vbe_edid *edid);
int edid_preferred_resolution(struct vbe_edid *edid, int *xres, int *yres);
void print_edid(struct vbe_edid *edid);

#endif	/* VBE_H_ */
