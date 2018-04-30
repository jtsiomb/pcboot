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
#ifndef ERRNO_H_
#define ERRNO_H_

#define EFOO			1
#define EAGAIN			2
#define EINVAL			3
#define ECHILD			4
#define EBUSY			5
#define ENOMEM			6
#define EIO				7
#define ENOENT			8
#define ENAMETOOLONG	9
#define ENOSPC			10
#define EPERM			11
#define ENOTDIR			12

#define EBUG			127	/* for missing features and known bugs */

int errno;

#endif	/* ERRNO_H_ */
