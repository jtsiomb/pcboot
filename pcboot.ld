OUTPUT_ARCH(i386)

SECTIONS {
	/* BIOS will load us at 0x7c000h */
	. = 0x7c000;

	.boot : { * (.boot); }
	.startup : { * (.startup); }
	.text : { * (.text); }
	.rodata : { * (.rodata); }
	.data : { * (.data); }

	.bss ALIGN(4): {
		_bss_start = .;
		* (.bss);
		. = ALIGN(4);
		_bss_end = .;
	}
	_bss_size = SIZEOF(.bss);

	_mem_start = .;
}
