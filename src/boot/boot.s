	.code16
	.section .boot,"a"

	mov $0x13, %ax
	int $0x10

	mov $1, %al
	mov $0x3c8, %dx
	outb %al, %dx
	mov $0x3c9, %dx
	mov $63, %al
	outb %al, %dx
	xor %al, %al
	outb %al, %dx
	outb %al, %dx

	mov $200, %ebx
	mov $0x00000101, %eax
	pushl $0xa000
	popl %es
	xor %di, %di
fill:
	mov %ebx, %ecx
	and $1, %ecx
	jnz 0f
	rol $16, %eax
0:	mov $80, %ecx
	rep stosl
	dec %ebx
	jnz fill

	cli
	hlt

	.org 510
	.byte 0x55
	.byte 0xaa
