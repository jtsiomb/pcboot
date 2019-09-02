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
#ifndef MATH_H_
#define MATH_H_

#define INFINITY	__builtin_inff()
#define NAN			__builtin_nanf

#define M_PI		3.141592653589793

#define sin(x)		__builtin_sin(x)
#define cos(x)		__builtin_cos(x)
#define tan(x)		__builtin_tan(x)
#define fabs(x)		__builtin_fabs(x)
#define fmod(x, y)	__builtin_fmod(x, y)
#define sqrt(x)		__builtin_sqrt(x)
#define atan2(y, x)	__builtin_atan2(y, x)

double pow(double x, double y);

#endif	/* MATH_H_ */
