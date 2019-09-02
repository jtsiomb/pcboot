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

	.code16
	.section .boot,"ax"

	.set stack_top, 0x7be0
	.set read_retries, 0x7be8
	.set drive_number, 0x7bec
	.set cursor_x, 0x7bee
	.set scratchbuf, 0x7bf0
	.set scratchbuf_size, 16

boot:
	cli
	# move stack to just below the code
	xor %ax, %ax
	mov %ax, %ss
	mov $stack_top, %sp
	# use the code segment for data access
	mov %cs, %ax
	mov %ax, %ds
	mov %ax, %es

	mov %dl, drive_number

	call get_drive_chs

	mov $loading_msg, %si
	call print_str

	# load the second stage boot loader and jump to it
	mov $_boot2_size, %eax
	call print_num

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

loading_msg: .asciz "\nLoad "
driveno_msg: .asciz "Drv:"

	.global sect_per_track
sect_per_track: .short 18
	.global num_cylinders
num_cylinders: .short 80
	.global num_heads
num_heads: .short 2
	.global heads_mask
heads_mask: .byte 1

get_drive_chs:
	mov $driveno_msg, %si
	call print_str
	xor %eax, %eax
	movb drive_number, %dl
	mov %dl, %al
	call print_num
	mov $10, %al
	call print_char

	mov $8, %ah
	int $0x13
	jnc ok
	ret

ok:	xor %eax, %eax
	mov %ch, %al
	mov %cl, %ah
	rol $2, %ah
	inc %ax
	and $0x3ff, %ax
	mov %ax, num_cylinders

	and $0x3f, %cx
	mov %cx, sect_per_track

	shr $8, %dx
	mov %dl, heads_mask
	inc %dx
	mov %dx, num_heads

	call print_num
	mov $47, %al
	call print_char
	mov %dx, %ax
	call print_num
	mov $47, %al
	call print_char
	mov %cx, %ax
	call print_num
	ret


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

# read_sector(sidx)
read_sector:
	push %bp
	mov %sp, %bp
	push %cx
	push %dx

	movw $3, read_retries

read_try:
	# calculate the track (sidx / sectors_per_track)
	mov 4(%bp), %ax

	xor %dx, %dx
	mov sect_per_track, %cx
	div %cx
	mov %ax, %cx
	# save the remainder
	push %dx
	# head in dh
	mov %cl, %dh
	and heads_mask, %dh
	# cylinder (track/heads) in ch [0-7] and cl[6,7]<-[8,9]
	push %dx
	xor %dx, %dx
	movw num_heads, %cx
	div %cx
	pop %dx
	mov %ax, %cx
	rol $8, %cx
	ror $2, %cl
	and $0xc0, %cl
	# sector num cl[0-5] is sidx % sectors_per_track + 1
	pop %ax
	inc %al
	or %al, %cl

	# ah = 2 (read), al = 1 sectors
	mov $0x0201, %ax
	movb drive_number, %dl
	int $0x13
	jnc read_ok

	# abort after 3 attempts
	decw read_retries
	jz read_fail

	# error detected, reset controller and retry
	xor %ah, %ah
	int $0x13
	jmp read_try

read_fail:
	mov 4(%bp), %ax
	jmp abort_read

read_ok:
	mov $46, %ax
	call print_char

	# increment es:bx accordingly (advance es if bx overflows)
	add $512, %bx
	jnc 0f
	mov %es, %ax
	add $4096, %ax
	mov %ax, %es

0:	pop %dx
	pop %cx
	pop %bp
	ret

str_read_error: .asciz "rderr:"

abort_read:
	mov $str_read_error, %si
	call print_str
	and $0xffff, %eax
	call print_num
	mov $10, %al
	call print_char
	hlt

	# expects string pointer in ds:si
print_str:
	pusha

0:	mov (%si), %al
	cmp $0, %al
	jz end
	call print_char
	inc %si
	jmp 0b

end:	popa
	ret

	# expects character in al
print_char:
	push %es

	cmp $10, %ax
	jnz 0f
	mov $32, %ax

0:	pushw $0xb800
	pop %es
	movw cursor_x, %di
	shl $1, %di

	mov %al, %es:(%di)
	movb $7, %es:1(%di)
	incw cursor_x

	pop %es
	ret

	# expects number in eax
	.global print_num
print_num:
	# save registers
	pusha

	movw $scratchbuf + scratchbuf_size, %si
	movb $0, (%si)
	mov $10, %ebx
convloop:
	xor %edx, %edx
	div %ebx
	add $48, %dl
	dec %si
	mov %dl, (%si)
	cmp $0, %eax
	jnz convloop

	call print_str

	# restore regs
	popa
	ret

	.org 510
	.byte 0x55
	.byte 0xaa
boot2_addr:
