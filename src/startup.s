# pcboot - bootable PC demo/game kernel
# Copyright (C) 2018-2019  John Tsiombikas <nuclear@member.fsf.org>
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

	.code32
	.section .startup,"ax"

	.extern _bss_start
	.extern _bss_end
	.extern pcboot_main
	.extern wait_vsync
	.extern kb_getkey

	.equ STACKTOP,0x80000

	# move the stack to the top of the conventional memory
	cli
	movl $STACKTOP, %esp
	# push a 0 ret-addr to terminate gdb backtraces
	pushl $0

	# zero the BSS section
	xor %eax, %eax
	mov $_bss_start, %edi
	mov $_bss_size, %ecx
	cmp $0, %ecx
	jz skip_bss_zero
	shr $2, %ecx
	rep stosl
skip_bss_zero:

	call pcboot_main
	# pcboot_main never returns
0:	cli
	hlt
	jmp 0b

	# this is called once after memory init, to move the protected mode
	# stack to the top of usable memory, to avoid interference from 16bit
	# programs (as much as possible)
	.global move_stack
move_stack:
	# calculate the currently used lowest address of the stack (rounded
	# down to 4-byte alignment), to see where to start copying
	mov %esp, %esi
	and $0xfffffffc, %esi
	# calculate the size we need to copy
	mov $STACKTOP, %ecx
	sub %esi, %ecx
	# load the destination address to edi
	mov 4(%esp), %edi
	sub %ecx, %edi
	# size in longwords
	shr $2, %ecx

	# change esp to the new stack
	mov $STACKTOP, %ecx
	sub %esp, %ecx
	mov 4(%esp), %eax
	mov %eax, %esp
	sub %ecx, %esp

	rep movsd
	ret

	.global logohack
logohack:
	pusha

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

	call wait_vsync

	# check for escape keypress
	call kb_getkey
	cmp $27, %eax
	jz 0f

	jmp frameloop

0:	popa
	ret

xval: .long 0
yval: .long 0
frameno: .long 0

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
