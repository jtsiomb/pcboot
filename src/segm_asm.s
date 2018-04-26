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

	.data
	.align 4
# memory reserved for setup_selectors
off:	.long 0
seg:	.short 0
# memory reserved for set_gdt
lim:	.short 0
addr:	.long 0

	.text
# setup_selectors(uint16_t code, uint16_t data)
# loads the requested selectors to all the selector registers
	.globl setup_selectors
setup_selectors:
	# set data selectors directly
	movl 8(%esp), %eax
	movw %ax, %ss
	movw %ax, %es
	movw %ax, %ds
	movw %ax, %gs
	movw %ax, %fs
	# set cs using a long jump
	movl 4(%esp), %eax
	movw %ax, (seg)
	movl $ldcs, (off)
	ljmp *off
ldcs:
	ret

# set_gdt(uint32_t addr, uint16_t limit)
# loads the GDTR with the new address and limit for the GDT
	.globl set_gdt
set_gdt:
	movl 4(%esp), %eax
	movl %eax, (addr)
	movw 8(%esp), %ax
	movw %ax, (lim)
	lgdt (lim)
	ret

# set_task_reg(uint16_t tss_selector)
# loads the TSS selector in the task register
	.globl set_task_reg
set_task_reg:
	mov 4(%esp), %eax
	ltr 4(%esp)
	ret
