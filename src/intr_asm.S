# pcboot - bootable PC demo/game kernel
# Copyright (C) 2018  John Tsiombikas <nuclear@member.fsf.org>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

	.data
	.align 4
	.short 0
idtr_desc:
lim:	.short 0
addr:	.long 0

	.text
/* void set_idt(uint32_t addr, uint16_t limit) */
	.global set_idt
set_idt:
	mov 4(%esp), %eax
	mov %eax, addr
	mov 8(%esp), %ax
	mov %ax, lim
	lidt (idtr_desc)
	ret

/* int get_intr_flag() */
	.global get_intr_flag
get_intr_flag:
	pushf
	popl %eax
	# bit 9 of eflags is IF
	shr $9, %eax
	and $1, %eax
	ret

/* void set_intr_flag(int onoff) */
	.global set_intr_flag
set_intr_flag:
	cmpl $0, 4(%esp)
	jz 0f
	sti
	ret
0:	cli
	ret

/* interrupt entry with error code macro
 * this macro generates an interrupt entry point for the
 * exceptions which include error codes in the stack frame
 */
	.macro ientry_err n name
	.globl intr_entry_\name
intr_entry_\name:
	pushl $\n
	jmp intr_entry_common
	.endm

/* interrupt entry without error code macro
 * this macro generates an interrupt entry point for the interrupts
 * and exceptions which do not include error codes in the stack frame
 * it pushes a dummy error code (0), to make the stack frame identical
 */
	.macro ientry_noerr n name
	.globl intr_entry_\name
intr_entry_\name:
	pushl $0
	pushl $\n
	jmp intr_entry_common
	.endm

/* common code used by all entry points. calls dispatch_intr()
 * defined in intr.c
 */
	.extern dispatch_intr
intr_entry_common:
	/* save general purpose registers */
	pusha
	call dispatch_intr
intr_ret_local:
	/* restore general purpose registers */
	popa
	/* remove error code and intr num from stack */
	add $8, %esp
	iret

/* special case for the timer interrupt, to avoid all the overhead of
 * going through the C interrupt dispatcher 250 times each second
 */
	.extern nticks
	.global intr_entry_fast_timer
intr_entry_fast_timer:
	incl nticks
	# signal end of interrupt
	push %eax
	mov $0x20, %al
	out %al, $0x20
	pop %eax
	iret

/* special case for IRQ 7 and IRQ 15, to catch spurious interrupts */
	.set PIC1_CMD, 0x20
	.set PIC2_CMD, 0xa0
	.set OCW2_EOI, 0x20
	.set OCW3_ISR, 0x0b

	.extern intr_entry_irq7
	.global irq7_entry_check_spurious
irq7_entry_check_spurious:
	push %eax
	mov $OCW3_ISR, %al
	out %al, $PIC1_CMD
	in $PIC1_CMD, %al
	and $0x80, %al
	pop %eax
	jnz intr_entry_irq7
	iret

	.extern intr_entry_irq15
	.global irq15_entry_check_spurious
irq15_entry_check_spurious:
	push %eax
	mov $OCW3_ISR, %al
	out %al, $PIC2_CMD
	in $PIC2_CMD, %al
	and $0x80, %al
	jnz 0f
	# it was spurious, send EOI to master PIC and iret
	mov $OCW2_EOI, %al
	out %al, $PIC1_CMD
	pop %eax
	iret
0:	pop %eax
	jmp intr_entry_irq15


/* XXX not necessary for now, just leaving it in in case it's useful
 * down the road.
 *
 * intr_ret is called by context_switch to return from the kernel
 * to userspace. The argument is a properly formed intr_frame
 * structure with the saved context of the new task.
 *
 * First thing to do is remove the return address pointing back
 * to context_switch, which then leaves us with a proper interrupt
 * stack frame, so we can jump right in the middle of the regular
 * interrupt return code above.
 */
	.global intr_ret
intr_ret:
	add $4, %esp
	jmp intr_ret_local

/* by including interrupts.h with ASM defined, the macros above
 * are expanded to generate all required interrupt entry points
 */
#define ASM
#include <intrtab.h>
