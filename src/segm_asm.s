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
