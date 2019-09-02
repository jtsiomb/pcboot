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
#ifndef FS_H_
#define FS_H_

#include <inttypes.h>

/* device ids for virtual filesystems */
enum {
	DEV_FLOPPY0		= 0,
	DEV_FLOPPY1		= 1,
	DEV_HDD0		= 0x80,
	DEV_HDD1		= 0x81,
	DEV_MEMDISK		= 0x10000
};

enum {
	FSTYPE_MEM,
	FSTYPE_FAT,

	NUM_FSTYPES
};

enum { FSNODE_FILE, FSNODE_DIR };

enum { FSSEEK_SET, FSSEEK_CUR, FSSEEK_END };

enum {
	FSO_CREATE	= 1,
	FSO_DIR		= 2,
	FSO_EXCL	= 4
};

struct filesys;
struct fs_node;
struct fs_dirent;

struct fs_operations {
	void (*destroy)(struct filesys *fs);

	struct fs_node *(*open)(struct filesys *fs, const char *path, unsigned int flags);
	void (*close)(struct fs_node *node);

	long (*fsize)(struct fs_node *node);
	int (*seek)(struct fs_node *node, int offs, int whence);
	long (*tell)(struct fs_node *node);
	int (*read)(struct fs_node *node, void *buf, int sz);
	int (*write)(struct fs_node *node, void *buf, int sz);

	int (*rewinddir)(struct fs_node *node);
	struct fs_dirent *(*readdir)(struct fs_node *node);

	int (*rename)(struct fs_node *node, const char *name);
	int (*remove)(struct fs_node *node);
};

struct filesys {
	int type;
	char *name;
	struct fs_operations *fsop;
	void *data;
};

struct fs_node {
	struct filesys *fs;
	int type;
	void *data;

	struct filesys *mnt;
};

struct fs_dirent {
	char *name;
	void *data;
	int type;
	long fsize;
};

struct filesys *rootfs;
struct fs_node *cwdnode;	/* current working directory node */

struct filesys *fs_mount(int dev, uint64_t start, uint64_t size, struct fs_node *parent);

int fs_chdir(const char *path);
char *fs_getcwd(void);

struct fs_node *fs_open(const char *path, unsigned int flags);
int fs_close(struct fs_node *node);

int fs_rename(struct fs_node *node, const char *name);
int fs_remove(struct fs_node *node);

long fs_filesize(struct fs_node *node);
int fs_seek(struct fs_node *node, int offs, int whence);
long fs_tell(struct fs_node *node);
int fs_read(struct fs_node *node, void *buf, int sz);
int fs_write(struct fs_node *node, void *buf, int sz);

int fs_rewinddir(struct fs_node *node);
struct fs_dirent *fs_readdir(struct fs_node *node);

/* fs utility functions */
char *fs_path_skipsep(char *s);

/* copies the current name into the namebuf, and returns a pointer to the
 * start of the next path component.
 */
char *fs_path_next(char *s, char *namebuf, int bufsz);

#endif	/* FS_H_ */
