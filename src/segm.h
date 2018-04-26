#ifndef SEGM_H_
#define SEGM_H_

#define SEGM_KCODE	1
#define SEGM_KDATA	2
#define SEGM_UCODE	3
#define SEGM_UDATA	4
#define SEGM_TASK	5

#ifndef ASM
void init_segm(void);

uint16_t selector(int idx, int rpl);

void set_tss(uint32_t addr);
#endif	/* ASM */


#endif	/* SEGM_H_ */
