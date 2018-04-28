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

	# standard C memset
	.global memset
memset:
	push %ebp
	mov %esp, %ebp
	push %edi

	mov 8(%ebp), %edi
	mov 12(%ebp), %al
	mov %al, %ah
	mov %ax, %cx
	shl $16, %eax
	mov %cx, %ax
	mov 16(%ebp), %ecx

	cmp $0, %ecx
	jz msdone

	mov %edi, %edx
	and $3, %edx
	jz msmain
	jmp *mspre_tab(,%edx,4)

mspre_tab: .long msmain, mspre1, mspre2, mspre3
mspre1:	stosb
	dec %ecx
mspre2:	stosb
	dec %ecx
mspre3:	stosb
	dec %ecx
	jz msdone

msmain:
	push %ecx
	shr $2, %ecx
	rep stosl
	pop %ecx

	and $3, %ecx
	jmp *mspost_tab(,%ecx,4)

mspost_tab: .long msdone, mspost1, mspost2, mspost3
mspost3:stosb
mspost2:stosb
mspost1:stosb

msdone:
	pop %edi
	pop %ebp
	ret


	# same as memset but copies 16bit values, and the count is the number
	# of 16bit values to copy, not bytes.
	.global memset16
memset16:
	push %ebp
	mov %esp, %ebp
	push %edi

	mov 8(%ebp), %edi
	mov 12(%ebp), %ax
	shl $16, %eax
	mov 12(%ebp), %ax
	mov 16(%ebp), %ecx

	cmp $0, %ecx
	jz ms16done

	mov %edi, %edx
	and $3, %edx
	jz ms16main
	jmp *ms16pre_tab(,%edx,4)

ms16pre_tab: .long ms16main, ms16pre1, ms16pre2, ms16pre3
ms16pre3:
	# unaligned by +3:
	# same as next one, but jump to ms16main instead of falling through
	mov %al, (%edi)
	mov %ah, -1(%edi,%ecx,2)
	rol $8, %eax
	inc %edi
	dec %ecx
	jz ms16done
	jmp ms16main
ms16pre1:
	# unaligned by +1:
	# - write low byte at edi
	# - write high byte at the end
	# - rotate by 8 for the rest
	# - decrement ecx
	mov %al, (%edi)
	mov %ah, -1(%edi,%ecx,2)
	rol $8, %eax
	inc %edi
	dec %ecx
	jz ms16done
ms16pre2:
	# unaligned by +2
	stosw
	dec %ecx
	jz ms16done

ms16main:
	push %ecx
	shr $1, %ecx
	rep stosl
	pop %ecx

	and $1, %ecx
	jz ms16done
	stosw
ms16done:

	pop %edi
	pop %ebp
	ret

	# standard C memcpy
	.global memcpy
memcpy:
	push %ebp
	mov %esp, %ebp
	push %edi
	push %esi

	mov 8(%ebp), %edi
	mov 12(%ebp), %esi
	mov 16(%ebp), %ecx

	cmp $0, %ecx
	jz mcdone

	mov %ecx, %edx
	shr $2, %ecx
	rep movsl

	and $3, %edx
	jmp *mcpost_tab(,%edx,4)

mcpost_tab: .long mcdone, mcpost1, mcpost2, mcpost3
mcpost3:movsb
mcpost2:movsb
mcpost1:movsb

mcdone:
	pop %esi
	pop %edi
	pop %ebp
	ret
