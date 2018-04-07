	.code32
	.section .startup

	.extern _bss_start
	.extern _bss_end

	xor %eax, %eax
	mov _bss_start, %edi
	mov _bss_size, %ecx
	shr $4, %ecx
	rep stosl
