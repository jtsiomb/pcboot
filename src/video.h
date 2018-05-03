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
#ifndef VIDEO_H_
#define VIDEO_H_

struct video_mode {
	int mode;
	int width, height;
	int bpp;
	int rbits, gbits, bbits;
	int rshift, gshift, bshift;
	unsigned int rmask, gmask, bmask;
};

void set_vga_mode(int mode);

void *set_video_mode(int mode);
int find_video_mode(int xsz, int ysz, int bpp);

int video_mode_count(void);
int video_mode_info(int n, struct video_mode *vid);

int get_color_bits(int *rbits, int *gbits, int *bbits);
int get_color_mask(unsigned int *rmask, unsigned int *gmask, unsigned int *bmask);
int get_color_shift(int *rshift, int *gshift, int *bshift);

/* defined in video_asm.s */
void wait_vsync(void);

#endif	/* VIDEO_H_ */
