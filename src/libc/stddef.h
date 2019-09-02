/*
pcboot - bootable PC demo/game kernel
Copyright (C) 2018-2019  John Tsiombikas <nuclear@member.fsf.org>

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
#ifndef STDDEF_H_
#define STDDEF_H_

#include <inttypes.h>

typedef int32_t ssize_t;
typedef uint32_t size_t;
typedef int wchar_t;

typedef int32_t ptrdiff_t;
typedef uint32_t intptr_t;

#define NULL	0

#endif	/* STDDEF_H_ */
