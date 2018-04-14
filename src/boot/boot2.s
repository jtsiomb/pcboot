# this is the second-stage boot loader
	.code16
	.section .boot2,"a"

	.extern print_num
	.extern ser_putchar

	mov $0x13, %ax
	int $0x10

	# copy palette
	mov $logo_pal, %si
	xor %cl, %cl

0:	xor %eax, %eax
	mov $0x3c8, %dx
	movb %cl, %al
	outb %al, %dx
	#DBG
	call print_num
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
	#DBG
	call print_num
	mov $32, %al
	call ser_putchar
	xor %eax, %eax
	####
	# green
	movb (%si), %al
	inc %si
	shr $2, %al
	outb %al, %dx
	#DBG
	call print_num
	mov $32, %al
	call ser_putchar
	xor %eax, %eax
	####
	# blue
	movb (%si), %al
	inc %si
	shr $2, %al
	outb %al, %dx
	#DBG
	call print_num
	mov $32, %al
	call ser_putchar
	mov $13, %al
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

logo_pal:
	.incbin "logo.pal"

	.align 16
logo_pix:
	.incbin "logo.raw"
