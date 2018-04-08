	.code16
	.section .boot,"a"

	cli
	cld
	# move stack to the top of 512k
	mov $0x7000, %ax
	mov %ax, %ss
	xor %sp, %sp
	# use the code segment for data access
	mov %cs, %ax
	mov %ax, %ds
	mov %ax, %es

	call clearscr

	mov $_boot2_size, %eax
	call print_num

	# load the second stage boot loader and jump to it
	mov $_boot2_size, %eax
	mov %eax, %ebx
	shr $9, %eax
	and $0x1ff, %ebx
	jz 0f
	inc %eax
0:	pushw %ax
	pushw $1
	# set es to the start of the destination buffer to allow reading in
	# full 64k chunks
	mov $boot2_addr, %bx
	shr $4, %bx
	mov %bx, %es
	xor %bx, %bx
	call readsect
	jmp boot2_addr

	cli
	hlt

	.set SECT_PER_TRACK, 18

	.set ARG_NSECT, 6
	.set ARG_SIDX, 4
	.set VAR_NTRACKS, -2

# readsect(first, num)
readsect:
	push %bp
	mov %sp, %bp
	sub $2, %sp

	# calculate how many tracks to read
	mov ARG_NSECT(%bp), %ax
	xor %dx, %dx
	mov $SECT_PER_TRACK, %cx
	div %cx
	cmp $0, %dx
	jz 0f
	inc %ax
0:	mov %ax, VAR_NTRACKS(%bp)

	xor %cx, %cx
0:	cmp VAR_NTRACKS(%bp), %cx
	jz 0f
	push %cx
	call read_track
	pop %cx
	jmp 0b
0:
	# TODO cont.
	pop %bp
	ret

read_track:
	ret

clearscr:
	push %es
	pushw $0xb800
	pop %es
	xor %eax, %eax
	xor %di, %di
	movl $500, %ecx
	rep stosl
	pop %es
	ret

print_num:
	# save es
	push %es

	xor %cx, %cx
	movw $numbuf, %si
	mov $10, %ebx

0:	xor %edx, %edx
	div %ebx
	add $48, %dl
	mov %dl, (%si)
	inc %si
	inc %cx
	cmp $0, %eax
	jnz 0b

	# print the backwards string
	pushw $0xb800
	pop %es
	xor %di, %di

0:	dec %si
	mov (%si), %al
	movb %al, %es:(%di)
	inc %di
	mov $7, %al
	movb %al, %es:(%di)
	inc %di
	dec %cx
	jnz 0b

	# restore es
	pop %es
	ret

numbuf: .space 10
	.org 510
	.byte 0x55
	.byte 0xaa

boot2_addr:
