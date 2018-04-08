OUTPUT_ARCH(i386)

SECTIONS {
	/* BIOS loads the boot code at 0000:7c00 */
	. = 0x7c00;

	.boot : { * (.boot); }

	/* second stage boot loader */
	.boot2 : { * (.boot2); }
	_boot2_size = SIZEOF(.boot2);

	/* main program will be loaded at 1MB by the second stage
	 * boot loader
	 */
	. = 1M;

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
