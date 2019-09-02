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
#include "math.h"

static double calc_pow(double x, double y, double precision);

double pow(double x, double y)
{
	if(y == 0.0 || y == -0.0) {
		return 1.0;
	}
	if(y == 1.0) {
		return x;
	}
	if(y == -INFINITY) {
		return fabs(x) < 1.0 ? INFINITY : 0.0;
	}
	if(y == INFINITY) {
		return fabs(x) < 1.0 ? 0.0 : INFINITY;
	}
	return calc_pow(x, y, 1e-6);
}

static double calc_pow(double x, double y, double precision)
{
	if(y < 0.0) {
		return 1.0 / calc_pow(x, -y, precision);
	}
	if(y >= 10.0) {
		double p = calc_pow(x, y / 2.0, precision / 2.0);
		return p * p;
	}
	if(y >= 1.0) {
		return x * calc_pow(x, y - 1.0, precision);
	}
	if(precision >= 1) {
		return __builtin_sqrt(x);
	}
	return __builtin_sqrt(calc_pow(x, y * 2.0, precision * 2.0));
}
