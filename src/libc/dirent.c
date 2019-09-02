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
#include <string.h>
#include <errno.h>
#include "dirent.h"
#include "fs.h"

struct DIR {
	struct fs_node *fsn;
	struct dirent dent;
};

DIR *opendir(const char *path)
{
	DIR *dir;
	struct fs_node *node;

	if(!path) {
		errno = EINVAL;
		return 0;
	}

	if(!(node = fs_open(path, 0))) {
		errno = ENOENT;
		return 0;
	}
	if(node->type != FSNODE_DIR) {
		errno = ENOTDIR;
		fs_close(node);
		return 0;
	}

	if(!(dir = malloc(sizeof *dir))) {
		errno = ENOMEM;
		fs_close(node);
		return 0;
	}
	dir->fsn = node;

	return dir;
}

int closedir(DIR *dir)
{
	if(!dir) {
		errno = EINVAL;
		return -1;
	}
	fs_close(dir->fsn);
	free(dir);
	return 0;
}

void rewinddir(DIR *dir)
{
	if(!dir) {
		errno = EINVAL;
		return;
	}
	fs_rewinddir(dir->fsn);
}

struct dirent *readdir(DIR *dir)
{
	struct fs_dirent *fsdent;

	if(!dir) {
		errno = EINVAL;
		return 0;
	}
	if(!(fsdent = fs_readdir(dir->fsn))) {
		return 0;
	}

	strcpy(dir->dent.d_name, fsdent->name);
	dir->dent.d_type = fsdent->type == FSNODE_DIR ? DT_DIR : DT_REG;
	dir->dent.d_fsize = fsdent->fsize;
	return &dir->dent;
}
