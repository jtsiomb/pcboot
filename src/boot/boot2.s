# this is the second-stage boot loader
	.code16
	.section .boot2,"a"

	mov $0x13, %ax
	int $0x10

	# copy palette
	mov $logo_pal, %si
	xor %cl, %cl

0:	mov $0x3c8, %dx
	movb %cl, %al
	outb %al, %dx
	inc %dx
	# red
	movb (%si), %al
	inc %si
	shr $2, %al
	outb %al, %dx
	# green
	movb (%si), %al
	inc %si
	shr $2, %al
	outb %al, %dx
	# blue
	movb (%si), %al
	inc %si
	shr $2, %al
	outb %al, %dx
	inc %cl
	jno 0b

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

logo_pal:
	.incbin "logo.pal"

	.align 16
logo_pix:
	.incbin "logo.raw"
