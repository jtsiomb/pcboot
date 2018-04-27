#ifndef VIDEO_H_
#define VIDEO_H_

void set_vga_mode(int mode);

/* defined in video_asm.s */
void wait_vsync(void);

#endif	/* VIDEO_H_ */
