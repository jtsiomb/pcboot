#ifndef STDARG_H_
#define STDARG_H_

/* Assumes that arguments are passed on the stack 4-byte aligned */

typedef int* va_list;

#define va_start(ap, last)	((ap) = (int*)&(last) + 1)
#define va_arg(ap, type)	(*(type*)(ap)++)
#define va_end(ap)

#endif	/* STDARG_H_ */
