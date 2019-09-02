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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <alloca.h>
#include "fs.h"
#include "mtab.h"
#include "panic.h"

struct filesys *fsfat_create(int dev, uint64_t start, uint64_t size);
struct filesys *fsmem_create(int dev, uint64_t start, uint64_t size);

static struct filesys *(*createfs[])(int, uint64_t, uint64_t) = {
	fsmem_create,
	fsfat_create
};

struct filesys *fs_mount(int dev, uint64_t start, uint64_t size, struct fs_node *parent)
{
	int i;
	struct filesys *fs;

	if(!parent && rootfs) {
		printf("fs_mount error: root filesystem already mounted!\n");
		return 0;
	}

	for(i=0; i<NUM_FSTYPES; i++) {
		if((fs = createfs[i](dev, start, size))) {
			if(!parent) {
				rootfs = fs;

				fs_chdir("/");
			} else {
				parent->mnt = fs;
				mtab_add(parent, fs);
			}
			return fs;
		}
	}

	printf("failed to mount filesystem dev: %d, start %llu\n", dev, (unsigned long long)start);
	return 0;
}

static char cwdpath[1024];
static char *cwdpath_end = cwdpath;

int fs_chdir(const char *path)
{
	struct fs_node *node;
	char *uppath = 0;
	int uplen;

	if(!path || !*path) {
		return -1;
	}

	if(strcmp(path, ".") == 0) {
		return 0;
	}
	if(strcmp(path, "..") == 0) {
		char *endptr;

		if(cwdpath_end <= cwdpath + 1) {
			return -1;
		}

		endptr = cwdpath + (cwdpath_end - cwdpath);
		while(endptr > cwdpath && *--endptr != '/');
		if(endptr == cwdpath) endptr++;

		uplen = endptr - cwdpath;
		uppath = alloca(uplen + 1);
		memcpy(uppath, cwdpath, uplen);
		uppath[uplen] = 0;

		path = uppath;
	}

	if(!(node = fs_open(path, 0))) {
		return -1;
	}
	if(node->type != FSNODE_DIR) {
		fs_close(node);
		return -1;
	}

	if(uppath) {
		memcpy(cwdpath, uppath, uplen + 1);
		cwdpath_end = cwdpath + uplen;

	} else {
		int len = strlen(path);
		if(cwdpath_end - cwdpath + len > sizeof cwdpath) {
			panic("fs_chdir: path too long: %s\n", path);
		}
		if(path[0] == '/') {
			memcpy(cwdpath, path, len + 1);
			cwdpath_end = cwdpath + len;
		} else {
			if(cwdpath_end > cwdpath + 1) {
				*cwdpath_end++ = '/';
			}
			memcpy(cwdpath_end, path, len + 1);
			cwdpath_end += len;
		}
	}

	fs_close(cwdnode);
	cwdnode = node;
	return 0;
}

char *fs_getcwd(void)
{
	return cwdpath;
}

/* TODO normalize path */
struct fs_node *fs_open(const char *path, unsigned int flags)
{
	struct filesys *fs;
	struct fs_node *node;

	if(!path || !*path) {
		return 0;
	}

	if(*path == '/') {
		fs = rootfs;
	} else {
		if(!cwdnode) return 0;
		fs = cwdnode->fs;
	}

	if(!(node = fs->fsop->open(fs, path, flags))) {
		return 0;
	}
	return node;
}

int fs_close(struct fs_node *node)
{
	struct fs_operations *fsop;

	if(!node) return -1;

	fsop = node->fs->fsop;
	fsop->close(node);
	return 0;
}

int fs_rename(struct fs_node *node, const char *name)
{
	struct fs_operations *fsop = node->fs->fsop;
	return fsop->rename(node, name);
}

int fs_remove(struct fs_node *node)
{
	struct fs_operations *fsop = node->fs->fsop;
	return fsop->remove(node);
}

long fs_filesize(struct fs_node *node)
{
	struct fs_operations *fsop = node->fs->fsop;

	if(node->type != FSNODE_FILE) {
		return -1;
	}
	return fsop->fsize(node);
}

int fs_seek(struct fs_node *node, int offs, int whence)
{
	struct fs_operations *fsop = node->fs->fsop;

	if(node->type != FSNODE_FILE) {
		return -1;
	}
	return fsop->seek(node, offs, whence);
}

long fs_tell(struct fs_node *node)
{
	struct fs_operations *fsop = node->fs->fsop;

	if(node->type != FSNODE_FILE) {
		return -1;
	}
	return fsop->tell(node);
}

int fs_read(struct fs_node *node, void *buf, int sz)
{
	struct fs_operations *fsop = node->fs->fsop;

	if(node->type != FSNODE_FILE) {
		return -1;
	}
	return fsop->read(node, buf, sz);
}

int fs_write(struct fs_node *node, void *buf, int sz)
{
	struct fs_operations *fsop = node->fs->fsop;

	if(node->type != FSNODE_FILE) {
		return -1;
	}
	return fsop->write(node, buf, sz);
}

int fs_rewinddir(struct fs_node *node)
{
	struct fs_operations *fsop = node->fs->fsop;

	if(node->type != FSNODE_DIR) {
		return -1;
	}
	return fsop->rewinddir(node);
}

struct fs_dirent *fs_readdir(struct fs_node *node)
{
	struct fs_operations *fsop = node->fs->fsop;

	if(node->type != FSNODE_DIR) {
		return 0;
	}
	return fsop->readdir(node);
}

/* fs utility functions */
char *fs_path_skipsep(char *s)
{
	while(*s == '/') s++;
	return s;
}

char *fs_path_next(char *s, char *namebuf, int bufsz)
{
	int len;
	char *ptr;

	ptr = s = fs_path_skipsep(s);

	while(*ptr && *ptr != '/') ptr++;

	if(namebuf) {
		len = ptr - s;
		if(len >= bufsz) len = bufsz - 1;

		memcpy(namebuf, s, len);
		namebuf[len] = 0;
	}

	return fs_path_skipsep(ptr);
}
