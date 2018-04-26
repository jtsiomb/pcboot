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
