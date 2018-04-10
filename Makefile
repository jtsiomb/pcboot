csrc = $(wildcard src/*.c) $(wildcard src/libc/*.c)
ssrc = $(wildcard src/*.s) $(wildcard src/libc/*.s) $(wildcard src/boot/*.s)
obj = $(csrc:.c=.o) $(ssrc:.s=.o)
dep = $(obj:.o=.d)
elf = test
bin = test.bin

warn = -pedantic -Wall
dbg = -g
inc = -Isrc -Isrc/libc

CFLAGS = $(ccarch) $(warn) $(dbg) -nostdinc -fno-builtin $(inc) $(def)
ASFLAGS = $(asarch) $(dbg) -nostdinc -fno-builtin $(inc)
LDFLAGS = $(ldarch) -T pcboot.ld -print-gc-sections


ifneq ($(shell uname -m), i386)
	ccarch = -m32
	asarch = --32
	ldarch = -m elf_i386
endif

floppy.img: $(bin)
	dd if=/dev/zero of=$@ bs=512 count=2880
	dd if=$< of=$@ conv=notrunc

$(bin): $(elf)
	objcopy -O binary $< $@

$(elf): $(obj)
	$(LD) -o $@ $(obj) -Map link.map $(LDFLAGS)

-include $(dep)

%.d: %.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: cleandep
cleandep:
	rm -f $(dep)

.PHONY: run
run: $(bin)
	qemu-system-i386 -fda floppy.img -serial file:serial.log
