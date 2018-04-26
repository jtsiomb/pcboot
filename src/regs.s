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

	.globl get_regs
get_regs:
	pushl %ebp
	movl %esp, %ebp

	pushl %edx
	movl 8(%ebp), %edx

	movl %eax, (%edx)
	movl %ebx, 4(%edx)
	movl %ecx, 8(%edx)
	
	# juggle edx
	movl %edx, %eax
	popl %edx
	movl %edx, 12(%eax)
	pushl %edx
	movl %eax, %edx
	
	# those two are pointless in a function
	movl %esp, 16(%edx)
	movl %ebp, 20(%edx)

	movl %esi, 24(%edx)
	movl %edi, 28(%edx)

	pushf
	popl %eax
	movl %eax, 32(%edx)

	movw %cs, 36(%edx)
	movw %ss, 40(%edx)
	movw %ds, 44(%edx)
	movw %es, 48(%edx)
	movw %fs, 52(%edx)
	movw %gs, 56(%edx)

	pushl %ebx
	movl %cr0, %ebx
	movl %ebx, 60(%edx)
	#movl %cr1, %ebx
	#movl %ebx, 64(%edx)
	movl %cr2, %ebx
	movl %ebx, 68(%edx)
	movl %cr3, %ebx
	movl %ebx, 72(%edx)
	popl %ebx

	popl %edx
	popl %ebp
	ret
