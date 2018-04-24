OUTPUT_ARCH(i386)

SECTIONS {
	/* BIOS loads the boot code at 0000:7c00 */
	. = 0x7c00;

	.boot : { * (.boot); }

	/* second stage boot loader */
	.boot2 : {
		* (.boot2);
		/* pad the boot loader to the next sector boundary */
		. = ALIGN(512);
	}
	_boot2_size = SIZEOF(.boot2);

	/* main program will be loaded at 1MB by the second stage
	 * boot loader
	 */
	. = 1M;
	_main_start = .;

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

	_main_size = . - _main_start;
	_mem_start = .;
}
