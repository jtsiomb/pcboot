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

# this is the second-stage boot loader
	.code16
	.section .boot2,"a"

	.extern ser_putchar

	mov $0x13, %ax
	int $0x10

	movb $10, %al
	call ser_putchar

	# copy palette
	mov $logo_pal, %si
	xor %cl, %cl

0:	xor %eax, %eax
	mov $0x3c8, %dx
	movb %cl, %al
	outb %al, %dx
	#### DBG
	call ser_print_num
	mov $58, %al
	call ser_putchar
	mov $32, %al
	call ser_putchar
	xor %eax, %eax
	####
	inc %dx
	# red
	movb (%si), %al
	inc %si
	shr $2, %al
	outb %al, %dx
	#### DBG
	call ser_print_num
	mov $32, %al
	call ser_putchar
	xor %eax, %eax
	####
	# green
	movb (%si), %al
	inc %si
	shr $2, %al
	outb %al, %dx
	#### DBG
	call ser_print_num
	mov $32, %al
	call ser_putchar
	xor %eax, %eax
	####
	# blue
	movb (%si), %al
	inc %si
	shr $2, %al
	outb %al, %dx
	#### DBG
	call ser_print_num
	mov $32, %al
	call ser_putchar
	mov $10, %al
	call ser_putchar
	xor %eax, %eax
	####
	add $1, %cl
	jnc 0b

	# copy pixels
	pushw $0xa000
	pop %es
	xor %di, %di
	mov $logo_pix, %eax
	shr $4, %eax
	mov %ax, %ds
	xor %si, %si
	mov $16000, %ecx
	rep movsl

	cli
	hlt

	# expects string pointer in ds:si
ser_print_str:
	pusha

0:	mov (%si), %al
	cmp $0, %al
	jz .Lend
	call ser_putchar
	inc %si
	jmp 0b

.Lend:	popa
	ret


	# expects number in eax
ser_print_num:
	# save registers
	pusha

	movw $numbuf + 16, %si
	movb $0, (%si)
	mov $10, %ebx
.Lconvloop:
	xor %edx, %edx
	div %ebx
	add $48, %dl
	dec %si
	mov %dl, (%si)
	cmp $0, %eax
	jnz .Lconvloop

	call ser_print_str

	# restore regs
	popa
	ret

numbuf: .space 16

logo_pal:
	.incbin "logo.pal"

	.align 16
logo_pix:
	.incbin "logo.raw"
