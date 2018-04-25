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

# this is the second-stage boot loader
	.code16
	.section .boot2,"ax"

	.set main_load_addr, 0x100000

	# make sure any BIOS call didn't re-enable interrupts
	cli

	# enter unreal mode
	call unreal

	movb $10, %al
	call ser_putchar

	call clearscr

	# enable A20 address line
	call enable_a20

	# load the whole program into memory starting at 1MB
	call load_main

	#mov $0x13, %ax
	#int $0x10

	# load initial GDT
	lgdt (gdt_lim)
	# load initial IDT
	lidt (idt_lim)

	# enter protected mode for the first time
	mov %cr0, %eax
	or $1, %eax
	mov %eax, %cr0
	# inter-segment jump to set cs selector to segment 1
	ljmp $0x8,$0f

	.code32
	# set all data selectors to segment 2
0:	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %ss
	mov %ax, %es
	mov %ax, %gs
	mov %ax, %fs

	jmp main_load_addr

	cli
0:	hlt
	jmp 0b



	.align 4
gdt_lim: .word 23
gdt_base:.long gdt

	.align 4
idt_lim: .word 111
idt_base:.long idt


	.align 8
gdt:	# 0: null segment
	.long 0
	.long 0
	# 1: code - base:0, lim:4g, G:4k, 32bit, avl, pres|app, dpl:0, type:code/non-conf/rd
	.long 0x0000ffff
	.long 0x00cf9a00
	# 2: data - base:0, lim:4g, G:4k, 32bit, avl, pres|app, dpl:0, type:data/rw
	.long 0x0000ffff
	.long 0x00cf9200


	.align 8
idt:	.space 104
	# trap gate 13: general protection fault
	.short prot_fault
	.short 0x8
	# type: trap, present, default
	.short 0x8f00
	.short 0

gpf_msg: .asciz "GP fault "

prot_fault:
	mov (%esp), %eax
	shr $3, %eax
	call print_num
	mov $64, %al
	call putchar
	mov 4(%esp), %eax
	call print_num
	mov $10, %al
	call putchar
	hlt

	.code16
unreal:
	# use the same GDT above, will use data segment: 2
	lgdt (gdt_lim)

	mov %cr0, %eax
	or $1, %ax
	mov %eax, %cr0
	jmp 0f

0:	mov $0x10, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss

	mov %cr0, %eax
	and $0xfffe, %ax
	mov %eax, %cr0

	xor %ax, %ax
	mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	mov %ax, %ss
	ret

mainsz_msg: .asciz "Main program size: "
mainsz_msg2: .asciz " ("
mainsz_msg3: .asciz " sectors)\n"

first_sect: .long 0
sect_left: .long 0
cur_track: .long 0
trk_sect: .long 0
dest_ptr: .long 0

load_main:
	movl $main_load_addr, dest_ptr

	# calculate first sector
	mov $_boot2_size, %eax
	add $511, %eax
	shr $9, %eax
	# add 1 to account for the boot sector
	inc %eax
	mov %eax, first_sect

	# calculate the first track (first_sect / sect_per_track)
	movzxw sect_per_track, %ecx
	xor %edx, %edx
	div %ecx
	mov %eax, cur_track
	# remainder is sector within track
	mov %edx, trk_sect

	mov $mainsz_msg, %esi
	call putstr
	mov $_main_size, %eax
	mov %eax, %ecx
	call print_num

	mov $mainsz_msg2, %esi
	call putstr

	# calculate sector count
	add $511, %eax
	shr $9, %eax
	mov %eax, sect_left

	call print_num
	mov $mainsz_msg3, %esi
	call putstr

	# read a whole track into the buffer (or partial first track)
ldloop:
	movzxw sect_per_track, %ecx
	sub trk_sect, %ecx
	push %ecx
	call read_track

	# debug: print the first 32bits of the track
	#mov buffer, %eax
	#call print_num
	#mov $10, %al
	#call putchar

	# copy to high memory
	mov $buffer, %esi
	mov dest_ptr, %edi
	mov (%esp), %ecx
	shl $9, %ecx
	add %ecx, dest_ptr
	shr $2, %ecx
	addr32 rep movsl

	incl cur_track
	# other than the first track which might be partial, all the rest start from 0
	movl $0, trk_sect

	pop %ecx
	sub %ecx, sect_left
	ja ldloop

	# the BIOS might have enabled interrupts
	cli

	# just in case we were loaded from floppy, turn all floppy motors off
	mov $0x3f2, %dx
	in %dx, %al
	and $0xf0, %al
	out %al, %dx

	mov $10, %ax
	call putchar

	ret

rdtrk_msg: .asciz "Reading track: "
rdcyl_msg: .asciz " - cyl: "
rdhead_msg: .asciz " head: "
rdsect_msg: .asciz " start sect: "
rdlast_msg: .asciz " ... "
rdok_msg: .asciz "OK\n"
rdfail_msg: .asciz "failed\n"

read_retries: .short 0

	.set drive_number, 0x7bec
read_track:
	# set es to the start of the destination buffer to allow reading in
	# full 64k chunks if necessary
	mov $buffer, %bx
	shr $4, %bx
	mov %bx, %es
	xor %ebx, %ebx

	movw $3, read_retries

read_try:
	# print track
	mov $rdtrk_msg, %esi
	call putstr
	mov cur_track, %eax
	call print_num
	mov $rdcyl_msg, %esi
	call putstr

	# calc cylinder (cur_track / num_heads) and head (cur_track % num_heads)
	mov cur_track, %eax
	movzxw num_heads, %ecx
	xor %edx, %edx
	div %ecx

	# print cylinder
	push %eax
	call print_num
	# print head
	mov $rdhead_msg, %esi
	call putstr
	movzx %dx, %eax
	call print_num
	pop %eax

	# head in dh
	mov %dl, %dh

	# cylinder low byte at ch and high bits at cl[7, 6]
	mov %al, %ch
	mov %ah, %cl
	and $3, %cl
	ror $2, %cl

	# print start sector
	mov $rdsect_msg, %esi
	call putstr
	mov trk_sect, %eax
	call print_num
	mov $rdlast_msg, %esi
	call putstr

	# start sector (1-based) in cl[0, 5]
	mov trk_sect, %al
	inc %al
	and $0x3f, %al
	or %al, %cl

	# number of sectors in al
	mov 2(%esp), %ax
	# call number (2) in ah
	mov $2, %ah
	# drive number in dl
	movb drive_number, %dl
	int $0x13
	jnc read_ok

	# abort after 3 attempts
	decw read_retries
	jz read_fail

	# error, reset controller and retry
	xor %ah, %ah
	int $0x13
	jmp read_try

read_fail:
	mov $rdfail_msg, %esi
	call putstr
	jmp abort_read

read_ok:
	mov $rdok_msg, %esi
	call putstr

	# reset es to 0 before returning
	xor %ax, %ax
	mov %ax, %es
	ret

str_read_error: .asciz "Read error while reading track: "

abort_read:
	mov $str_read_error, %esi
	call putstr
	mov cur_track, %eax
	call print_num
	mov $10, %al
	call putchar

	cli
0:	hlt
	jmp 0b


	# better print routines, since we're not constrainted by the 512b of
	# the boot sector.
	.global cursor_x
	.global cursor_y
cursor_x: .long 0
cursor_y: .long 0

putchar:
	pushal
	call ser_putchar

	cmp $10, %al
	jnz 0f
	call video_newline
	jmp 1f

0:	push %eax
	mov cursor_y, %eax
	mov $80, %ecx
	mul %ecx
	add cursor_x, %eax
	mov %eax, %ebx
	pop %eax

	mov $0xb8000, %edx

	# this looks retarded. in nasm: [ebx * 2 + edx]
	mov %al, (%edx, %ebx, 2)
	movb $7, 1(%edx, %ebx, 2)
	incl cursor_x
	cmpl $80, cursor_x
	jnz 1f
	call video_newline

1:	popal
	ret
	
	# expects string pointer in esi
putstr:
	mov (%esi), %al
	cmp $0, %al
	jz 0f
	call putchar
	inc %esi
	jmp putstr
0:	ret

	# expects number in eax
print_num:
	# save registers
	pushal

	mov $numbuf + 16, %esi
	movb $0, (%esi)
	mov $10, %ebx
convloop:
	xor %edx, %edx
	div %ebx
	add $48, %dl
	dec %esi
	mov %dl, (%esi)
	cmp $0, %eax
	jnz convloop

	call putstr

	# restore regs
	popal
	ret


video_newline:
	movl $0, cursor_x
	incl cursor_y
	cmpl $25, cursor_y
	jnz 0f
	call scrollup
	decl cursor_y
0:	ret

scrollup:
	pushal
	# move 80 * 24 lines from b80a0 -> b8000
	mov $0xb8000, %edi
	mov $0xb80a0, %esi
	mov $960, %ecx
	addr32 rep movsl
	# clear last line (b8f00)
	mov $0xb8f00, %edi
	xor %eax, %eax
	mov $40, %ecx
	addr32 rep stosl
	popal
	ret

clearscr:
	mov $0xb8000, %edi
	xor %eax, %eax
	mov $1000, %ecx
	addr32 rep stosl
	ret

	.set UART_DATA, 0x3f8
	.set UART_LSTAT, 0x3fd
	.set LST_TREG_EMPTY, 0x20

ser_putchar:
	push %dx

	cmp $10, %al
	jnz 0f
	push %ax
	mov $13, %al
	call ser_putchar
	pop %ax

0:	mov %al, %ah
	# wait until the transmit register is empty
	mov $UART_LSTAT, %dx
wait:	in %dx, %al
	and $LST_TREG_EMPTY, %al
	jz wait
	mov $UART_DATA, %dx
	mov %ah, %al
	out %al, %dx

	pop %dx
	ret



ena20_msg: .asciz "A20 line enabled\n"

enable_a20:
	call test_a20
	jnc a20done
	call enable_a20_kbd
	call test_a20
	jnc a20done
	call enable_a20_fast
	call test_a20
	jnc a20done
	# keep trying ... we can't do anything useful without A20 anyway
	jmp enable_a20
a20done:
	mov $ena20_msg, %esi
	call putstr
	ret

	# CF = 1 if A20 test fails (not enabled)
test_a20:
	mov $0x07c000, %ebx
	mov $0x17c000, %edx
	movl $0xbaadf00d, (%ebx)
	movl $0xaabbcc42, (%edx)
	subl $0xbaadf00d, (%ebx)
	ret

	# enable A20 line through port 0x92 (fast A20)
enable_a20_fast:
	mov $ena20_fast_msg, %esi
	call putstr

	in $0x92, %al
	or $2, %al
	out %al, $0x92
	ret

ena20_fast_msg: .asciz "Attempting fast A20 enable\n"


	# enable A20 line through the keyboard controller
	.set KBC_DATA_PORT, 0x60
	.set KBC_CMD_PORT, 0x64
	.set KBC_STATUS_PORT, 0x64
	.set KBC_CMD_RD_OUTPORT, 0xd0
	.set KBC_CMD_WR_OUTPORT, 0xd1

	.set KBC_STAT_OUT_RDY, 0x01
	.set KBC_STAT_IN_FULL, 0x02

enable_a20_kbd:
	mov $ena20_kbd_msg, %esi
	call putstr

	call kbc_wait_write
	mov $KBC_CMD_WR_OUTPORT, %al
	out %al, $KBC_CMD_PORT
	call kbc_wait_write
	mov $0xdf, %al
	out %al, $KBC_DATA_PORT
	ret

ena20_kbd_msg: .asciz "Attempting KBD A20 enable\n"

	# wait until the keyboard controller is ready to accept another byte
kbc_wait_write:
	in $KBC_STATUS_PORT, %al
	and $KBC_STAT_IN_FULL, %al
	jnz kbc_wait_write
	ret

numbuf: .space 16

	.align 16
buffer:
