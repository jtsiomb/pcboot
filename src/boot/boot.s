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

	mov %dl, drive_number

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
	call read_sect
	jmp boot2_addr

	cli
	hlt

	.set SECT_PER_TRACK, 18

	.set ARG_NSECT, 6
	.set ARG_SIDX, 4

# read_sect(first, num)
read_sect:
	push %bp
	mov %sp, %bp

	mov ARG_SIDX(%bp), %ax
	mov ARG_NSECT(%bp), %cx

	jmp 1f
0:	push %ax
	call read_sector
	add $2, %sp
1:	cmp ARG_NSECT(%bp), %cx
	jnz 0b

	pop %bp
	ret

# read_sector(sidx)
read_sector:
	push %bp
	mov %sp, %bp
	push %cx
	push %dx

	# calculate the track (sidx / sectors_per_track)
	mov 4(%bp), %ax
	xor %dx, %dx
	mov $SECT_PER_TRACK, %cx
	div %cx
	mov %ax, %cx
	# save the remainder in ax
	mov %dx, %ax
	# head in dh
	mov %cl, %dh
	and $1, %dh
	# cylinder (track/2) in ch [0-7] and cl[6,7]<-[8,9]
	rol $7, %cx
	ror $2, %cl
	and $0xc0, %cl
	# sector num cl[0-5] is sidx % sectors_per_track (saved in ax)
	inc %al
	or %al, %cl 
	# ah = 2 (read), al = 1 sectors
	mov $0x0201, %ax
	movb drive_number, %dl
	int $0x13

	# increment es:bx accordingly (advance es if bx overflows)
	add $512, %bx
	jno 0f
	mov %es, %ax
	add $4096, %ax
	mov %ax, %es

	pop %dx
	pop %cx
	pop %bp
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

# expects number in eax
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

drive_number: .byte 0
numbuf: .space 10
	.org 510
	.byte 0x55
	.byte 0xaa

boot2_addr:
