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
