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
	.section .boot2,"a"

	# make sure any BIOS call didn't re-enable interrupts
	cli

	# just in case we were loaded from floppy, turn all floppy motors off
	call motor_off

	mov $0x13, %ax
	int $0x10

	# load initial GDT/IDT
	lgdt (gdt_lim)
	lidt (idt_lim)
	# enable protection
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

	movb $10, %al
	call ser_putchar

	call clearscr

	mov $hello, %esi
	call putstr

	# enable A20 line
	call enable_a20

	call logohack

	cli
	hlt

hello: .asciz "Switched to 32bit\n"

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

	# better print routines, since we're not constrainted by the 512b of
	# the boot sector.
cursor_x: .long 0
cursor_y: .long 0

putchar:
	pusha
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

1:	popa
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
	pusha

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
	popa
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
	pusha
	# move 80 * 24 lines from b8050 -> b8000
	mov $0xb8000, %edi
	mov $0xb8050, %esi
	mov $480, %ecx
	rep movsl
	# clear last line (b8780)
	mov $0xb8780, %edi
	xor %eax, %eax
	mov $20, %ecx
	rep stosl
	popa
	ret

clearscr:
	mov $0xb8000, %edi
	xor %eax, %eax
	mov $500, %ecx
	rep stosl
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


motor_off:
	mov $0x3f2, %dx
	in %dx, %al
	and $0xf0, %al
	out %al, %dx
	ret

logohack:
	# copy palette
	mov $logo_pal, %esi
	xor %cl, %cl

0:	xor %eax, %eax
	mov $0x3c8, %dx
	movb %cl, %al
	outb %al, %dx
	inc %dx
	# red
	movb (%esi), %al
	inc %esi
	shr $2, %al
	outb %al, %dx
	# green
	movb (%esi), %al
	inc %esi
	shr $2, %al
	outb %al, %dx
	# blue
	movb (%esi), %al
	inc %esi
	shr $2, %al
	outb %al, %dx
	add $1, %cl
	jnc 0b

	# copy pixels
	mov $sintab, %ebp
	mov $logo_pix, %esi
frameloop:
	mov $0xa0000, %edi
	movl $0, yval
yloop:
	movl $0, xval
xloop:
	# calc src scanline address -> ebx
	xor %ecx, %ecx
	mov yval, %ebx
	shl $2, %ebx
	add frameno, %ebx
	and $0xff, %ebx
	mov (%ebp, %ebx), %cl
	shr $5, %ecx

	mov yval, %eax
	add %ecx, %eax
	# bounds check
	cmp $200, %eax
	jl 0f
	mov $199, %eax

0:	mov %eax, %ebx
	shl $8, %eax
	shl $6, %ebx
	add %eax, %ebx

	# calc src x offset -> eax
	xor %ecx, %ecx
	mov xval, %eax
	shl $2, %eax
	add frameno, %eax
	and $0xff, %eax
	mov (%ebp, %eax), %cl
	shr $5, %ecx

	mov xval, %eax
	add %ecx, %eax
	# bounds check
	cmp $320, %eax
	jl 0f
	mov $319, %eax

0:	add %eax, %ebx
	mov (%ebx, %esi), %al

	mov %al, (%edi)
	inc %edi

	incl xval
	cmpl $320, xval
	jnz xloop

	incl yval
	cmpl $200, yval
	jnz yloop

	incl frameno

	# wait vsync
	mov $0x3da, %dx
0:	in %dx, %al
	and $8, %al
	jnz 0b
0:	in %dx, %al
	and $8, %al
	jz 0b
	jmp frameloop

xval: .long 0
yval: .long 0
frameno: .long 0

numbuf: .space 16

logo_pal:
	.incbin "logo.pal"

	.align 16
logo_pix:
	.incbin "logo.raw"

sintab:
	.byte 127
	.byte 130
	.byte 133
	.byte 136
	.byte 139
	.byte 143
	.byte 146
	.byte 149
	.byte 152
	.byte 155
	.byte 158
	.byte 161
	.byte 164
	.byte 167
	.byte 170
	.byte 173
	.byte 176
	.byte 179
	.byte 182
	.byte 184
	.byte 187
	.byte 190
	.byte 193
	.byte 195
	.byte 198
	.byte 200
	.byte 203
	.byte 205
	.byte 208
	.byte 210
	.byte 213
	.byte 215
	.byte 217
	.byte 219
	.byte 221
	.byte 224
	.byte 226
	.byte 228
	.byte 229
	.byte 231
	.byte 233
	.byte 235
	.byte 236
	.byte 238
	.byte 239
	.byte 241
	.byte 242
	.byte 244
	.byte 245
	.byte 246
	.byte 247
	.byte 248
	.byte 249
	.byte 250
	.byte 251
	.byte 251
	.byte 252
	.byte 253
	.byte 253
	.byte 254
	.byte 254
	.byte 254
	.byte 254
	.byte 254
	.byte 255
	.byte 254
	.byte 254
	.byte 254
	.byte 254
	.byte 254
	.byte 253
	.byte 253
	.byte 252
	.byte 251
	.byte 251
	.byte 250
	.byte 249
	.byte 248
	.byte 247
	.byte 246
	.byte 245
	.byte 244
	.byte 242
	.byte 241
	.byte 239
	.byte 238
	.byte 236
	.byte 235
	.byte 233
	.byte 231
	.byte 229
	.byte 228
	.byte 226
	.byte 224
	.byte 221
	.byte 219
	.byte 217
	.byte 215
	.byte 213
	.byte 210
	.byte 208
	.byte 205
	.byte 203
	.byte 200
	.byte 198
	.byte 195
	.byte 193
	.byte 190
	.byte 187
	.byte 184
	.byte 182
	.byte 179
	.byte 176
	.byte 173
	.byte 170
	.byte 167
	.byte 164
	.byte 161
	.byte 158
	.byte 155
	.byte 152
	.byte 149
	.byte 146
	.byte 143
	.byte 139
	.byte 136
	.byte 133
	.byte 130
	.byte 127
	.byte 124
	.byte 121
	.byte 118
	.byte 115
	.byte 111
	.byte 108
	.byte 105
	.byte 102
	.byte 99
	.byte 96
	.byte 93
	.byte 90
	.byte 87
	.byte 84
	.byte 81
	.byte 78
	.byte 75
	.byte 72
	.byte 70
	.byte 67
	.byte 64
	.byte 61
	.byte 59
	.byte 56
	.byte 54
	.byte 51
	.byte 49
	.byte 46
	.byte 44
	.byte 41
	.byte 39
	.byte 37
	.byte 35
	.byte 33
	.byte 30
	.byte 28
	.byte 26
	.byte 25
	.byte 23
	.byte 21
	.byte 19
	.byte 18
	.byte 16
	.byte 15
	.byte 13
	.byte 12
	.byte 10
	.byte 9
	.byte 8
	.byte 7
	.byte 6
	.byte 5
	.byte 4
	.byte 3
	.byte 3
	.byte 2
	.byte 1
	.byte 1
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 0
	.byte 1
	.byte 1
	.byte 2
	.byte 3
	.byte 3
	.byte 4
	.byte 5
	.byte 6
	.byte 7
	.byte 8
	.byte 9
	.byte 10
	.byte 12
	.byte 13
	.byte 15
	.byte 16
	.byte 18
	.byte 19
	.byte 21
	.byte 23
	.byte 25
	.byte 26
	.byte 28
	.byte 30
	.byte 33
	.byte 35
	.byte 37
	.byte 39
	.byte 41
	.byte 44
	.byte 46
	.byte 49
	.byte 51
	.byte 54
	.byte 56
	.byte 59
	.byte 61
	.byte 64
	.byte 67
	.byte 70
	.byte 72
	.byte 75
	.byte 78
	.byte 81
	.byte 84
	.byte 87
	.byte 90
	.byte 93
	.byte 96
	.byte 99
	.byte 102
	.byte 105
	.byte 108
	.byte 111
	.byte 115
	.byte 118
	.byte 121
	.byte 124
