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
#ifndef LIMITS_H_
#define LIMITS_H_

#define CHAR_BIT	8

#define SHRT_MIN	(-32768)
#define SHRT_MAX	32767
#define INT_MIN		(-2147483648)
#define INT_MAX		2147483647
#define LONG_MIN	(-2147483648)
#define LONG_MAX	2147483647

#define USHRT_MAX	65535
#define UINT_MAX	0xffffffff
#define ULONG_MAX	0xffffffff

#define PATH_MAX	256

#endif	/* LIMITS_H_ */
