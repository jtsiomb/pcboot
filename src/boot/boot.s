	.code16
	.section .boot,"a"

	cli
	cld
	# move stack to just below the code
	xor %ax, %ax
	mov %ax, %ss
	mov $0x7c00, %sp
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
	inc %ax
0:	pushw %ax
	pushw $1
	# set es to the start of the destination buffer to allow reading in
	# full 64k chunks
	mov $boot2_addr, %bx
	shr $4, %bx
	mov %bx, %es
	xor %bx, %bx
	call read_sectors
	jmp boot2_addr

	cli
	hlt

	.set SECT_PER_TRACK, 18

	.set ARG_NSECT, 6
	.set ARG_SIDX, 4

# read_sectors(first, num)
read_sectors:
	push %bp
	mov %sp, %bp

	mov ARG_SIDX(%bp), %ax
	xor %cx, %cx

	jmp 1f
0:	push %ax
	call read_sector
	pop %ax
	inc %ax
	inc %cx
1:	cmp ARG_NSECT(%bp), %cx
	jnz 0b

	pop %bp
	ret

	.set VAR_ATTEMPTS, -2

# read_sector(sidx)
read_sector:
	push %bp
	mov %sp, %bp
	sub $2, %sp
	push %cx
	push %dx

	movw $3, VAR_ATTEMPTS(%bp)

.Lread_try:
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
	jnc .Lread_ok

	# abort after 3 attempts
	decw VAR_ATTEMPTS(%bp)
	jz .Lread_fail

	# error detected, reset controller and retry
	xor %ah, %ah
	int $0x13
	jmp .Lread_try

.Lread_fail:
	mov 4(%bp), %ax
	jmp abort_read

.Lread_ok:
	# increment es:bx accordingly (advance es if bx overflows)
	add $512, %bx
	jno 0f
	mov %es, %ax
	add $4096, %ax
	mov %ax, %es

0:	pop %dx
	pop %cx
	add $2, %sp
	pop %bp
	ret

str_read_error: .asciz "Failed to read sector: "

abort_read:
	push %ax
	mov $str_read_error, %si
	call print_str

	xor %eax, %eax
	pop %ax
	call print_num

	cli
	hlt

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

cursor_x: .byte 0

# expects string pointer in ds:si
print_str:
	push %es
	pushw $0xb800
	pop %es
	xor %di, %di
	movb $0, cursor_x

0:	mov (%si), %al
	mov %al, %es:(%di)
	movb $7, %es:1(%di)
	inc %si
	add $2, %di
	incb cursor_x
	cmp $0, %al
	jnz 0b

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
	movb cursor_x, %al
	xor %ah, %ah
	shl $1, %ax
	mov %ax, %di

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
