	.text

	.align 4
	.short 0
saved_idtr:
idtlim:	.short 0
idtaddr:.long 0

	.short 0
saved_gdtr:
gdtlim:	.short 0
gdtaddr:.long 0

	.short 0
rmidt:	.short 0x3ff
	.long 0

	# drop back to real mode to set video mode hack
	.global set_mode13h
set_mode13h:
	cli
	#sgdt (saved_gdtr)
	sidt (saved_idtr)
	lidt (rmidt)

	mov %cr0, %eax
	and $0xfffe, %ax
	mov %eax, %cr0
	jmp 0f

0:	xor %ax, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss

	#mov $0x13, %ax
	#int $0x10

	mov %cr0, %eax
	or $1, %ax
	mov %eax, %cr0
	jmp 0f

0:	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %ss

	sidt (saved_idtr)
	sti
	ret
