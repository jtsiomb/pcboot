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

	.extern ser_putchar

	mov $0x13, %ax
	int $0x10

	movb $10, %al
	call ser_putchar

	# copy palette
	mov $logo_pal, %si
	xor %cl, %cl

0:	xor %eax, %eax
	mov $0x3c8, %dx
	movb %cl, %al
	outb %al, %dx
	#### DBG
	call ser_print_num
	mov $58, %al
	call ser_putchar
	mov $32, %al
	call ser_putchar
	xor %eax, %eax
	####
	inc %dx
	# red
	movb (%si), %al
	inc %si
	shr $2, %al
	outb %al, %dx
	#### DBG
	call ser_print_num
	mov $32, %al
	call ser_putchar
	xor %eax, %eax
	####
	# green
	movb (%si), %al
	inc %si
	shr $2, %al
	outb %al, %dx
	#### DBG
	call ser_print_num
	mov $32, %al
	call ser_putchar
	xor %eax, %eax
	####
	# blue
	movb (%si), %al
	inc %si
	shr $2, %al
	outb %al, %dx
	#### DBG
	call ser_print_num
	mov $32, %al
	call ser_putchar
	mov $10, %al
	call ser_putchar
	xor %eax, %eax
	####
	add $1, %cl
	jnc 0b

	# copy pixels
	pushw $0xa000
	pop %es
	mov $logo_pix, %eax
	shr $4, %eax
	#mov %ax, %ds
	mov %ax, %gs
	#mov $16000, %ecx
	#rep movsl

	mov $sintab, %eax
	shr $4, %eax
	mov %ax, %fs

.Lframeloop:
	xor %di, %di

	movw $0, yval
.Lyloop:
	movw $0, xval
.Lxloop:
	# calc src scanline address -> bx
	mov yval, %bx
	shl $2, %bx
	add frameno, %bx
	xor %bh, %bh
	mov %fs:(%bx), %cl
	xor %ch, %ch
	shr $5, %cx

	mov yval, %ax
	add %cx, %ax
	# bounds check
	cmp $200, %ax
	jl 0f
	mov $199, %ax

0:	mov %ax, %bx
	shl $8, %ax
	shl $6, %bx
	add %ax, %bx

	# calc src x offset -> si
	mov xval, %ax
	shl $2, %ax
	add frameno, %ax
	xor %ah, %ah
	mov %ax, %si
	mov %fs:(%si), %cl
	xor %ch, %ch
	shr $5, %cx

	mov xval, %ax
	add %cx, %ax
	# bounds check
	cmp $320, %ax
	jl 0f
	mov $319, %ax

0:	mov %ax, %si

	mov %gs:(%bx, %si), %al

	mov %al, %es:(%di)
	inc %di

	incw xval
	cmpw $320, xval
	jnz .Lxloop

	incw yval
	cmpw $200, yval
	jnz .Lyloop

	incw frameno

	# wait vsync
	mov $0x3da, %dx
0:	in %dx, %al
	and $8, %al
	jnz 0b
0:	in %dx, %al
	and $8, %al
	jz 0b
	jmp .Lframeloop

	cli
	hlt

xval: .word 0
yval: .word 0
frameno: .word 0

	# expects string pointer in ds:si
ser_print_str:
	pusha

0:	mov (%si), %al
	cmp $0, %al
	jz .Lend
	call ser_putchar
	inc %si
	jmp 0b

.Lend:	popa
	ret


	# expects number in eax
ser_print_num:
	# save registers
	pusha

	movw $numbuf + 16, %si
	movb $0, (%si)
	mov $10, %ebx
.Lconvloop:
	xor %edx, %edx
	div %ebx
	add $48, %dl
	dec %si
	mov %dl, (%si)
	cmp $0, %eax
	jnz .Lconvloop

	call ser_print_str

	# restore regs
	popa
	ret

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
