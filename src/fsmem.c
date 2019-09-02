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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <alloca.h>
#include "fs.h"
#include "panic.h"

#define MAX_NAME	120

struct memfs_node;
struct memfs_file;
struct memfs_dir;

struct memfs {
	struct memfs_node *rootdir;
};

struct memfs_dir {
	struct memfs_node *clist, *ctail;
	struct memfs_node *cur;
};

struct odir {
	struct memfs_dir *dir;
	struct memfs_node *cur;
	struct fs_dirent dent;
};

struct memfs_file {
	char *data;
	long size, max_size;
};

struct ofile {
	struct memfs_file *file;
	long cur_pos;
};

struct memfs_node {
	union {
		struct memfs_file file;
		struct memfs_dir dir;
	};
	int type;
	char name[MAX_NAME + 4];
	struct memfs_node *parent;
	struct memfs_node *next;
	struct fs_node *fsnode;	/* we need it for crossing mounts in fs_open */
};


static void destroy(struct filesys *fs);

static struct fs_node *open(struct filesys *fs, const char *path, unsigned int flags);
static void close(struct fs_node *node);
static long fsize(struct fs_node *node);
static int seek(struct fs_node *node, int offs, int whence);
static long tell(struct fs_node *node);
static int read(struct fs_node *node, void *buf, int sz);
static int write(struct fs_node *node, void *buf, int sz);
static int rewinddir(struct fs_node *node);
static struct fs_dirent *readdir(struct fs_node *node);
static int rename(struct fs_node *node, const char *name);
static int remove(struct fs_node *node);

static struct fs_node *create_fsnode(struct filesys *fs, struct memfs_node *n);

static struct memfs_node *alloc_node(int type);
static void free_node(struct memfs_node *node);

static struct memfs_node *find_entry(struct memfs_node *dir, const char *name);
static void add_child(struct memfs_node *dir, struct memfs_node *n);


static struct fs_operations fs_mem_ops = {
	destroy,
	open, close,

	fsize,
	seek, tell,
	read, write,

	rewinddir, readdir,

	rename, remove
};


struct filesys *fsmem_create(int dev, uint64_t start, uint64_t size)
{
	struct filesys *fs;
	struct memfs *memfs;

	if(dev != DEV_MEMDISK) {
		return 0;
	}

	if(!(memfs = malloc(sizeof *memfs))) {
		panic("MEMFS: create failed to allocate memory\n");
	}
	if(!(memfs->rootdir = alloc_node(FSNODE_DIR))) {
		panic("MEMFS: failed to allocate root dir\n");
	}

	if(!(fs = malloc(sizeof *fs))) {
		panic("MEMFS: failed to allocate memory for the filesystem structure\n");
	}
	fs->type = FSTYPE_MEM;
	fs->name = 0;
	fs->fsop = &fs_mem_ops;
	fs->data = memfs;

	return fs;
}

static void destroy(struct filesys *fs)
{
	struct memfs *memfs = fs->data;
	free_node((struct memfs_node*)memfs->rootdir);
	free(memfs);
	free(fs);
}

static struct fs_node *open_mount(struct filesys *fs, const char *path, unsigned int flags)
{
	char *newpath;

	newpath = alloca(strlen(path) + 2);
	newpath[0] = '/';
	strcpy(newpath + 1, path);

	return fs->fsop->open(fs, newpath, flags);
}

#define NODE_IS_MNTPT(n)	((n)->fsnode && (n)->fsnode->mnt)

static struct fs_node *open(struct filesys *fs, const char *path, unsigned int flags)
{
	struct memfs_node *node, *parent;
	struct memfs *memfs = fs->data;
	char name[MAX_NAME + 1];

	if(path[0] == '/') {
		node = memfs->rootdir;
		path = fs_path_skipsep((char*)path);
	} else {
		if(cwdnode->fs->type != FSTYPE_MEM) {
			return 0;
		}
		node = (struct memfs_node*)((struct odir*)cwdnode->data)->dir;
	}
	assert(node->type == FSNODE_DIR);

	while(*path) {
		if(node->type != FSNODE_DIR) {
			/* we have more path components, yet the last one wasn't a dir */
			errno = ENOTDIR;
			return 0;
		}
		/* check if it's another filesystem hanging off this directory, and if
		 * so, recursively call that open function to complete the operation
		 */
		if(NODE_IS_MNTPT(node)) {
			return open_mount(node->fsnode->mnt, path, flags);
		}

		path = fs_path_next((char*)path, name, sizeof name);
		parent = node;

		if(!(node = find_entry(node, name))) {
			if(*path || !(flags & FSO_CREATE)) {
				errno = ENOENT;
				return 0;
			}
			/* create and add */
			if(!(node = alloc_node((flags & FSO_DIR) ? FSNODE_DIR : FSNODE_FILE))) {
				errno = ENOMEM;
				return 0;
			}
			strcpy(node->name, name);
			add_child(parent, node);
			return create_fsnode(fs, node);
		}
	}

	/* we need to check for mount points here too, because the check in the loop
	 * above is not going to be reached when the mount point is the last part of
	 * the path string (for instance opendir("/mnt/foo"))
	 */
	if(NODE_IS_MNTPT(node)) {
		return open_mount(node->fsnode->mnt, path, flags);
	}

	if(flags & FSO_EXCL) {
		errno = EEXIST;
		return 0;
	}
	return create_fsnode(fs, node);
}

static struct fs_node *create_fsnode(struct filesys *fs, struct memfs_node *n)
{
	struct fs_node *fsn;
	struct ofile *of;
	struct odir *od;

	if(!(fsn = calloc(1, sizeof *fsn))) {
		errno = ENOMEM;
		return 0;
	}

	if(n->type == FSNODE_FILE) {
		if(!(of = malloc(sizeof *of))) {
			errno = ENOMEM;
			free(fsn);
			return 0;
		}
		of->file = &n->file;
		of->cur_pos = 0;
		fsn->data = of;
	} else {
		if(!(od = malloc(sizeof *od))) {
			errno = ENOMEM;
			free(fsn);
			return 0;
		}
		od->dir = &n->dir;
		od->cur = n->dir.clist;
		fsn->data = od;
	}

	if(!n->fsnode) {
		n->fsnode = fsn;
	}

	fsn->fs = fs;
	fsn->type = n->type;
	return fsn;
}

static void close(struct fs_node *node)
{
	if(!node) return;

	free(node->data);	/* free the copy of memfs_node allocated by create_fsnode */
	free(node);
}

static long fsize(struct fs_node *node)
{
	struct ofile *of;
	if(!node || node->type != FSNODE_FILE) {
		return -1;
	}
	of = node->data;
	return of->file->size;
}

static int seek(struct fs_node *node, int offs, int whence)
{
	struct ofile *of;
	long new_pos;

	if(!node || node->type != FSNODE_FILE) {
		return -1;
	}

	of = node->data;

	switch(whence) {
	case FSSEEK_SET:
		new_pos = offs;
		break;

	case FSSEEK_CUR:
		new_pos = of->cur_pos + offs;
		break;

	case FSSEEK_END:
		new_pos = of->file->size + offs;
		break;

	default:
		return -1;
	}

	if(new_pos < 0) new_pos = 0;

	of->cur_pos = new_pos;
	return 0;
}

static long tell(struct fs_node *node)
{
	struct ofile *of;

	if(!node || node->type != FSNODE_FILE) {
		return -1;
	}
	of = node->data;
	return of->cur_pos;
}

static int read(struct fs_node *node, void *buf, int sz)
{
	struct ofile *of;

	if(!node || !buf || sz < 0 || node->type != FSNODE_FILE) {
		return -1;
	}

	of = node->data;

	if(sz > of->file->size - of->cur_pos) {
		sz = of->file->size - of->cur_pos;
	}
	memcpy(buf, of->file->data + of->cur_pos, sz);
	of->cur_pos += sz;
	return sz;
}

static int write(struct fs_node *node, void *buf, int sz)
{
	struct ofile *of;
	int total_sz, new_max_sz;
	void *tmp;

	if(!node || !buf || sz < 0 || node->type != FSNODE_FILE) {
		return -1;
	}

	of = node->data;
	total_sz = of->cur_pos + sz;
	if(total_sz > of->file->max_size) {
		if(total_sz < of->file->max_size * 2) {
			new_max_sz = of->file->max_size * 2;
		} else {
			new_max_sz = total_sz;
		}
		if(!(tmp = realloc(of->file->data, new_max_sz))) {
			errno = ENOSPC;
			return -1;
		}
		of->file->data = tmp;
		of->file->max_size = new_max_sz;
	}

	memcpy(of->file->data + of->cur_pos, buf, sz);
	of->cur_pos += sz;
	if(of->cur_pos > of->file->size) of->file->size = of->cur_pos;
	return sz;
}

static int rewinddir(struct fs_node *node)
{
	struct odir *od;

	if(!node || node->type != FSNODE_DIR) {
		return -1;
	}

	od = node->data;
	od->cur = od->dir->clist;
	return 0;
}

static struct fs_dirent *readdir(struct fs_node *node)
{
	struct odir *od;
	struct memfs_node *n;
	struct fs_dirent *fsd;

	if(!node || node->type != FSNODE_DIR) {
		return 0;
	}

	od = node->data;
	fsd = &od->dent;

	n = od->cur;
	if(!n) return 0;

	od->cur = od->cur->next;

	fsd->name = n->name;
	fsd->data = 0;
	fsd->type = n->type;
	fsd->fsize = n->file.size;

	return fsd;
}

static int rename(struct fs_node *node, const char *name)
{
	struct memfs_node *n = (struct memfs_node*)((struct odir*)node->data)->dir;
	strncpy(n->name, name, MAX_NAME);
	n->name[MAX_NAME] = 0;
	return 0;
}

static int remove(struct fs_node *node)
{
	int res = -1;
	struct odir *od = 0;
	struct ofile *of = 0;
	struct memfs_node *n, *par, *prev, dummy;

	if(node->type == FSNODE_DIR) {
		od = node->data;
		n = (struct memfs_node*)od->dir;

		if(n->dir.clist) {
			errno = EEXIST;
			return -1;
		}
	} else {
		of = node->data;
		n = (struct memfs_node*)of->file;
	}
	par = n->parent;

	if(!par) {
		errno = EBUSY;
		return -1;
	}

	dummy.next = par->dir.clist;
	prev = &dummy;
	while(prev->next) {
		if(prev->next == n) {
			if(par->dir.ctail == n) {
				par->dir.ctail = prev;
			}
			prev->next = n->next;
			free_node(n);
			res = 0;
			break;
		}
		prev = prev->next;
	}
	par->dir.clist = dummy.next;
	return res;
}

static struct memfs_node *alloc_node(int type)
{
	struct memfs_node *node;

	if(!(node = calloc(1, sizeof *node))) {
		return 0;
	}
	node->type = type;
	return node;
}

static void free_node(struct memfs_node *node)
{
	if(!node) return;

	switch(node->type) {
	case FSNODE_FILE:
		free(node->file.data);
		break;

	case FSNODE_DIR:
		while(node->dir.clist) {
			struct memfs_node *n = node->dir.clist;
			node->dir.clist = n->next;
			free_node(n);
		}
		break;
	}
}

static struct memfs_node *find_entry(struct memfs_node *dnode, const char *name)
{
	struct memfs_node *n;

	if(strcmp(name, ".") == 0) return dnode;
	if(strcmp(name, "..") == 0) return dnode->parent;

	n = dnode->dir.clist;
	while(n) {
		if(strcasecmp(n->name, name) == 0) {
			return n;
		}
		n = n->next;
	}
	return 0;
}

static void add_child(struct memfs_node *dnode, struct memfs_node *n)
{
	if(dnode->dir.clist) {
		dnode->dir.ctail->next = n;
		dnode->dir.ctail = n;
	} else {
		dnode->dir.clist = dnode->dir.ctail = n;
	}
	n->parent = dnode;
}
