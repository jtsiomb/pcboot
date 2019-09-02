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
#include <stdlib.h>
#include "mtab.h"

int mtab_add(struct fs_node *node, struct filesys *fs)
{
	struct mount *m;

	if(!(m = malloc(sizeof *m))) {
		return -1;
	}
	m->mpt = node;
	m->fs = fs;
	m->next = 0;

	if(mnt_list) {
		mnt_tail->next = m;
		mnt_tail = m;
	} else {
		mnt_list = mnt_tail = m;
	}
	mnt_count++;
	return 0;
}

int mtab_remove_node(struct fs_node *node)
{
	int res = -1;
	struct mount dummy;
	struct mount *prev, *m;

	dummy.next = mnt_list;
	prev = &dummy;

	while(prev->next) {
		m = prev->next;
		if(m->mpt == node) {
			prev->next = m->next;
			free(m);
			res = 0;
			break;
		}
		prev = prev->next;
	}
	mnt_list = dummy.next;
	return res;
}
