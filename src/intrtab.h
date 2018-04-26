/*
pcboot - bootable PC demo/game kernel
Copyright (C) 2018  John Tsiombikas <nuclear@member.fsf.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY, without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifdef ASM
/* included from intr_asm.S */
#define INTR_ENTRY_EC(n, name)		ientry_err n, name
#define INTR_ENTRY_NOEC(n, name)	ientry_noerr n, name
#else
/* included from intr.c inside init_intr() */
#define INTR_ENTRY_EC(n, name)		\
	void intr_entry_##name(void);	\
	set_intr_entry(n, intr_entry_##name);
#define INTR_ENTRY_NOEC(n, name)	INTR_ENTRY_EC(n, name)
#endif	/* ASM */

/* faults/traps/aborts (plus NMI) */
INTR_ENTRY_NOEC(0, div)
INTR_ENTRY_NOEC(1, debug)
INTR_ENTRY_NOEC(2, nmi)
INTR_ENTRY_NOEC(3, bp)
INTR_ENTRY_NOEC(4, overflow)
INTR_ENTRY_NOEC(5, bound)
INTR_ENTRY_NOEC(6, ill)
INTR_ENTRY_NOEC(7, nodev)
INTR_ENTRY_EC(8, dfault)
INTR_ENTRY_NOEC(9, copseg)
INTR_ENTRY_EC(10, tss)
INTR_ENTRY_EC(11, segpres)
INTR_ENTRY_EC(12, stack)
INTR_ENTRY_EC(13, prot)
INTR_ENTRY_EC(14, page)
INTR_ENTRY_NOEC(15, reserved)
INTR_ENTRY_NOEC(16, fpu)
INTR_ENTRY_EC(17, align)
INTR_ENTRY_NOEC(18, mce)
INTR_ENTRY_NOEC(19, sse)
/* redirected IRQs */
INTR_ENTRY_NOEC(32, irq0)
INTR_ENTRY_NOEC(33, irq1)
INTR_ENTRY_NOEC(34, irq2)
INTR_ENTRY_NOEC(35, irq3)
INTR_ENTRY_NOEC(36, irq4)
INTR_ENTRY_NOEC(37, irq5)
INTR_ENTRY_NOEC(38, irq6)
INTR_ENTRY_NOEC(39, irq7)
INTR_ENTRY_NOEC(40, irq8)
INTR_ENTRY_NOEC(41, irq9)
INTR_ENTRY_NOEC(42, irq10)
INTR_ENTRY_NOEC(43, irq11)
INTR_ENTRY_NOEC(44, irq12)
INTR_ENTRY_NOEC(45, irq13)
INTR_ENTRY_NOEC(46, irq14)
INTR_ENTRY_NOEC(47, irq15)
/* system call interrupt */
INTR_ENTRY_NOEC(128, syscall)
/* default interrupt */
INTR_ENTRY_NOEC(255, default)
