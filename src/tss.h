#ifndef TSS_H_
#define TSS_H_

#include <inttypes.h>

struct task_state {
	uint32_t prev_task;
	uint32_t esp0, ss0;	/* we only ever set these two values */
	uint32_t esp1, ss1;
	uint32_t esp2, ss2;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax, ecx, edx, ebx;
	uint32_t esp, ebp, esi, edi;
	uint32_t es, cs, ss, ds, fs, gs;
	uint32_t ldt_sel;
	uint16_t trap, iomap_addr;
} __attribute__((packed));

#endif	/* TSS_H_ */
