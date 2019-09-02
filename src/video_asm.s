# pcboot - bootable PC demo/game kernel
# Copyright (C) 2018  John Tsiombikas <nuclear@member.fsf.org>
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
	.text

	.global wait_vsync
wait_vsync:
	mov $0x3da, %dx
0:	in %dx, %al
	and $8, %al
	jnz 0b
0:	in %dx, %al
	and $8, %al
	jz 0b
	ret

	.global set_pal_entry
set_pal_entry:
	mov 4(%esp), %al
	mov $0x3c8, %dx
	out %al, %dx
	inc %dx
	mov 8(%esp), %al
	shr $2, %al
	out %al, %dx
	mov 12(%esp), %al
	shr $2, %al
	out %al, %dx
	mov 16(%esp), %al
	shr $2, %al
	out %al, %dx
	ret
