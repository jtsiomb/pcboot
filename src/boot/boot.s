	.code16
	.section .boot,"a"

	.set scratchbuf, 0x7b00

boot:
	cli
	cld
	# move stack to just below the code
	xor %ax, %ax
	mov %ax, %ss
	mov $0x7b00, %sp
	# use the code segment for data access
	mov %cs, %ax
	mov %ax, %ds
	mov %ax, %es

	mov %dl, drive_number

	call setup_serial

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
str_rdsec_msg: .asciz "rdsec: "
str_cyl_msg: .asciz " C "
str_head_msg: .asciz " H "
str_sec_msg: .asciz " S "

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
	mov $str_rdsec_msg, %si
	call print_str_num16

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

	mov $str_cyl_msg, %si
	mov %cx, %ax
	rol $2, %al
	and $3, %al
	ror $8, %ax
	call print_str_num16

	mov $str_head_msg, %si
	xor %ax, %ax
	mov %dh, %al
	call print_str_num16

	mov $str_sec_msg, %si
	mov %cl, %al
	and $0x3f, %ax
	call print_str_num16

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
	# DBG print first dword
	mov $str_read_error + 4, %si
	mov %es:(%bx), %eax
	call print_str_num

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

str_read_error: .asciz "err read sect: "

abort_read:
	mov $str_read_error, %si
	call print_str_num16
	hlt

cursor_x: .byte 0

	# prints a string (ds:si) followed by a number (eax)
print_str_num:
	push %eax
	call print_str
	pop %eax
	call print_num
	mov $13, %al
	call ser_putchar
	mov $10, %al
	call ser_putchar
	ret

print_str_num16:
	push %eax
	and $0xffff, %eax
	call print_str_num
	pop %eax
	ret

	# expects string pointer in ds:si
print_str:
	push %es
	pusha

	pushw $0xb800
	pop %es
	xor %di, %di
	movb $0, cursor_x

	push %si

0:	mov (%si), %al
	mov %al, %es:(%di)
	movb $7, %es:1(%di)
	inc %si
	add $2, %di
	incb cursor_x
	cmp $0, %al
	jnz 0b

	pop %si
	call ser_putstr

	popa
	pop %es
	ret

	# expects number in eax
print_num:
	# save registers
	push %es
	pusha

	xor %cx, %cx
	movw $scratchbuf, %si
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
	call ser_putchar
	inc %di
	mov $7, %al
	movb %al, %es:(%di)
	inc %di
	dec %cx
	jnz 0b

	# restore regs
	popa
	pop %es
	ret

	.set UART_DATA, 0x3f8
	.set UART_DIVLO, 0x3f8
	.set UART_DIVHI, 0x3f9
	.set UART_FIFO, 0x3fa
	.set UART_LCTL, 0x3fb
	.set UART_MCTL, 0x3fc
	.set UART_LSTAT, 0x3fd
	.set DIV_9600, 115200 / 9600
	.set LCTL_8N1, 0x03
	.set LCTL_DLAB, 0x80
	.set FIFO_ENABLE, 0x01
	.set FIFO_SEND_CLEAR, 0x04
	.set FIFO_RECV_CLEAR, 0x02
	.set MCTL_DTR, 0x01
	.set MCTL_RTS, 0x02
	.set MCTL_OUT2, 0x08
	.set LST_TREG_EMPTY, 0x20

setup_serial:
	# set clock divisor
	mov $LCTL_DLAB, %al
	mov $UART_LCTL, %dx
	out %al, %dx
	mov $DIV_9600, %ax
	mov $UART_DIVLO, %dx
	out %al, %dx
	shr $8, %ax
	mov $UART_DIVHI, %dx
	out %al, %dx
	# set format 8n1
	mov $LCTL_8N1, %al
	mov $UART_LCTL, %dx
	out %al, %dx
	# clear and enable fifo
	mov $FIFO_ENABLE, %al
	or $FIFO_SEND_CLEAR, %al
	or $FIFO_RECV_CLEAR, %al
	mov $UART_FIFO, %dx
	out %al, %dx
	# assert RTS and DTR
	mov $MCTL_DTR, %al
	or $MCTL_RTS, %al
	or $MCTL_OUT2, %al
	mov $UART_MCTL, %dx
	out %al, %dx
	ret

	# expects a character in al
ser_putchar:
	push %dx

	mov %al, %ah
	# wait until the transmit register is empty
	mov $UART_LSTAT, %dx
0:	in %dx, %al
	and $LST_TREG_EMPTY, %al
	jz 0b
	mov $UART_DATA, %dx
	mov %ah, %al
	out %al, %dx

	pop %dx
	ret

	# expects a string in ds:si
ser_putstr:
	mov (%si), %al
	cmp $0, %al
	jz 0f
	call ser_putchar
	inc %si
	jmp ser_putstr
0:	ret
	

drive_number: .byte 0
	.org 510
	.byte 0x55
	.byte 0xaa

boot2_addr:
