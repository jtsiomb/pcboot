#ifndef ASMOPS_H_
#define ASMOPS_H_

#include <inttypes.h>

#define enable_intr() asm volatile("sti")
#define disable_intr() asm volatile("cli")
#define halt_cpu() asm volatile("hlt")

#define CALLER_EIP(eip) \
	asm volatile( \
		"mov (%%esp), %0\n\t" \
		: "=a" ((uint32_t)eip))

static inline uint8_t inb(uint16_t port)
{
	uint8_t res;
	asm volatile (
		"inb %1, %0\n\t"
		: "=a" (res)
		: "dN" (port));
	return res;
}

static inline uint16_t inw(uint16_t port)
{
	uint16_t res;
	asm volatile (
		"inw %1, %0\n\t"
		: "=a" (res)
		: "dN" (port));
	return res;
}

static inline uint32_t inl(uint16_t port)
{
	uint32_t res;
	asm volatile (
		"inl %1, %0\n\t"
		: "=a" (res)
		: "dN" (port));
	return res;
}

#define outb(src, port) \
	asm volatile( \
		"outb %0, %1\n\t" \
		:: "a" ((uint8_t)(src)), "dN" ((uint16_t)(port)))

#define outw(src, port) \
	asm volatile( \
		"outw %0, %1\n\t" \
		:: "a" ((uint16_t)(src)), "dN" ((uint16_t)(port)))

#define outl(src, port) \
	asm volatile( \
		"outl %0, %1\n\t" \
		:: "a" ((uint32_t)(src)), "dN" ((uint16_t)(port)))

#define iodelay() outb(0, 0x80)


#endif	/* ASMOPS_H_ */
