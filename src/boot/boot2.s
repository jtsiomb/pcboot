# this is the second-stage boot loader
	.code16
	.section .boot2,"a"

	mov $0x13, %ax
	int $0x10

	pushw $63
	pushw $63
	pushw $0
	pushw $0xff
	call set_palette
	add $8, %sp

	pushw $0xa000
	pop %es
	xor %di, %di
	mov $16000, %ecx
	mov $0xffffffff, %eax
	rep stosl

	hlt

set_palette:
	mov %sp, %bp
	mov $0x3c8, %dx
	movw 2(%bp), %ax
	outb %al, %dx
	inc %dx
	movw 4(%bp), %ax
	outb %al, %dx
	movw 6(%bp), %ax
	outb %al, %dx
	movw 8(%bp), %ax
	outb %al, %dx
	ret
