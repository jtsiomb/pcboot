# pcboot - bootable PC demo/game kernel
# Copyright (C) 2018-2019  John Tsiombikas <nuclear@member.fsf.org>
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

	.section .lowtext,"ax"

	.code32
	.align 4
	# place to save the protected mode IDTR pseudo-descriptor
	# with sidt, so that it can be restored before returning
	.short 0
saved_idtr:
idtlim:	.short 0
idtaddr:.long 0
	# real mode IDTR pseudo-descriptor pointing to the IVT at addr 0
	.short 0
rmidt:	.short 0x3ff
	.long 0

saved_esp: .long 0
saved_ebp: .long 0
saved_eax: .long 0
saved_es: .word 0
saved_ds: .word 0
saved_flags: .word 0
saved_pic1_mask: .byte 0
saved_pic2_mask: .byte 0

	# drop back to unreal mode to call 16bit interrupt
	.global int86
int86:
	push %ebp
	mov %esp, %ebp
	pushal
	cli
	# save protected mode IDTR and replace it with the real mode vectors
	sidt (saved_idtr)
	lidt (rmidt)

	# save PIC masks
	pushl $0
	call get_pic_mask
	add $4, %esp
	mov %al, saved_pic1_mask
	pushl $1
	call get_pic_mask
	add $4, %esp
	mov %al, saved_pic2_mask

	# modify the int instruction. do this here before the
	# cs-load jumps, to let them flush the instruction cache
	mov $int_op, %ebx
	movb 8(%ebp), %al
	movb %al, 1(%ebx)

	# long jump to load code selector for 16bit code (6)
	ljmp $0x30,$0f
0:
	.code16
	# disable protection
	mov %cr0, %eax
	and $0xfffe, %ax
	mov %eax, %cr0
	# load cs <- 0
	ljmp $0,$0f
0:	# zero data segments
	xor %ax, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	nop

	# load registers from the int86regs struct
	# point esp to the regs struct to load registers with popa/popf
	mov %esp, saved_esp
	mov %ebp, saved_ebp
	mov 12(%ebp), %esp
	popal
	popfw
	pop %es
	pop %ds
	# ignore fs and gs for now, don't think I'm going to need them

	# move to the real-mode stack, accessible from ss=0
	# just in case the BIOS call screws up our unreal mode
	mov $0x7be0, %esp

	# call 16bit interrupt
int_op:	int $0
	# BIOS call might have enabled interrupts, cli for good measure
	cli

	# save all registers that we'll clobber before having the
	# chance to populate the int86regs structure
	mov %eax, saved_eax
	mov %ds, saved_ds
	mov %es, saved_es
	pushfw
	popw %ax
	mov %ax, saved_flags

	# re-enable protection
	mov %cr0, %eax
	or $1, %ax
	mov %eax, %cr0
	# long jump to load code selector for 32bit code (1)
	ljmp $0x8,$0f
0:
	.code32
	# set data selector (2) to all segment regs
	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss
	nop

	# point the esp to our regs struct, to fill it with pusha/pushf
	mov saved_ebp, %ebp
	mov 12(%ebp), %esp
	add $38, %esp
	mov saved_ds, %ax
	pushw %ax
	mov saved_es, %ax
	pushw %ax
	# grab the flags and replace the carry bit from the saved flags
	pushfw
	popw %ax
	and $0xfffe, %ax
	or saved_flags, %ax
	pushw %ax
	mov saved_eax, %eax
	pushal
	mov saved_esp, %esp

	# restore 32bit interrupt descriptor table
	lidt (saved_idtr)

	# restore PIC configuration
	call init_pic

	# restore IRQ masks
	movzbl saved_pic1_mask, %eax
	push %eax
	pushl $0
	call set_pic_mask
	add $8, %esp

	movzbl saved_pic2_mask, %eax
	push %eax
	pushl $1
	call set_pic_mask
	add $8, %esp

	# keyboard voodoo: with some BIOS implementations, after returning from
	# int13, there's (I guess) leftover data in the keyboard port and we
	# can't receive any more keyboard interrupts afterwards. Reading from
	# the keyboard data port (60h) once, seems to resolve this. And it's
	# cheap enough, so why not... I give up.
	push %eax
	in $0x60, %al
	pop %eax

	sti
	popal
	pop %ebp
	ret
