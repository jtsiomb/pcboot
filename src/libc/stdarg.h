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
#ifndef STDARG_H_
#define STDARG_H_

/* Assumes that arguments are passed on the stack 4-byte aligned */

typedef int* va_list;

#define va_start(ap, last)	((ap) = (int*)&(last) + 1)
#define va_arg(ap, type)	(*(type*)(ap)++)
#define va_end(ap)

#endif	/* STDARG_H_ */
