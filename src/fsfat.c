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
#include <inttypes.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include "fs.h"
#include "bootdev.h"
#include "boot.h"
#include "panic.h"

#define MAX_NAME	195

#define DIRENT_UNUSED	0xe5

#define DENT_IS_NULL(dent)	(((unsigned char*)(dent))[0] == 0)
#define DENT_IS_UNUSED(dent)	(((unsigned char*)(dent))[0] == DIRENT_UNUSED)

#define ATTR_RO		0x01
#define ATTR_HIDDEN	0x02
#define ATTR_SYSTEM	0x04
#define ATTR_VOLID	0x08
#define ATTR_DIR	0x10
#define ATTR_ARCHIVE	0x20
#define ATTR_LFN	0xf


enum { FAT12, FAT16, FAT32, EXFAT };
static const char *typestr[] = { "fat12", "fat16", "fat32", "exfat" };

struct fat_dirent;
struct fat_dir;

struct fatfs {
	int type;
	int dev;
	uint64_t start_sect;
	uint32_t size;
	int cluster_size;
	int fat_size;
	uint32_t fat_sect;
	uint32_t root_sect;
	int root_size;
	uint32_t first_data_sect;
	uint32_t num_data_sect;
	uint32_t num_clusters;
	char label[12];

	void *fat;
	struct fat_dir *rootdir;
	unsigned int clust_mask;
	int clust_shift;
};

struct bparam {
	uint8_t jmp[3];
	unsigned char fmtid[8];
	uint16_t sect_bytes;
	uint8_t cluster_size;
	uint16_t reserved_sect;
	uint8_t num_fats;
	uint16_t num_dirent;
	uint16_t num_sectors;
	uint8_t medium;
	uint16_t fat_size;
	uint16_t track_size;
	uint16_t num_heads;
	uint32_t num_hidden;
	uint32_t num_sectors32;
} __attribute__((packed));

struct bparam_ext16 {
	uint8_t driveno;
	uint8_t ntflags;
	uint8_t signature;
	uint32_t volume_id;
	char label[11];
	char sysid[8];
} __attribute__((packed));

struct bparam_ext32 {
	uint32_t fat_size;
	uint16_t flags;
	uint16_t version;
	uint32_t root_clust;
	uint16_t fsinfo_sect;
	uint16_t backup_boot_sect;
	char junk[12];
	uint8_t driveno;
	uint8_t ntflags;
	uint8_t signature;
	uint32_t volume_id;
	char label[11];
	char sysid[8];
} __attribute__((packed));

struct fat_dirent {
	char name[11];
	uint8_t attr;
	uint8_t junk;
	uint8_t ctime_tenths;
	uint16_t ctime_halfsec;
	uint16_t ctime_date;
	uint16_t atime_date;
	uint16_t first_cluster_high;	/* 0 for FAT12 and FAT16 */
	uint16_t mtime_hsec;
	uint16_t mtime_date;
	uint16_t first_cluster_low;
	uint32_t size_bytes;
} __attribute__((packed));

struct fat_lfnent {
	uint8_t seq;
	uint16_t part1[5];
	uint8_t attr;
	uint8_t type;
	uint8_t csum;
	uint16_t part2[6];
	uint16_t zero;
	uint16_t part3[2];
} __attribute__((packed));


struct fat_dir {
	struct fatfs *fatfs;

	struct fat_dirent *ent;
	int max_nent;

	struct fs_dirent *fsent;
	int fsent_size;
	int cur_ent;

	int ref;
};

struct fat_file {
	struct fat_dirent ent;
	int32_t first_clust;
	int64_t cur_pos;
	int32_t cur_clust;	/* cluster number corresponding to cur_pos */

	char *clustbuf;
	int buf_valid;
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

static struct fat_dir *load_dir(struct fatfs *fs, struct fat_dirent *dent);
static void parse_dir_entries(struct fat_dir *dir);
static void free_dir(struct fat_dir *dir);

static struct fat_file *init_file(struct fatfs *fatfs, struct fat_dirent *dent);
static void free_file(struct fat_file *file);

static int read_sectors(int dev, uint64_t sidx, int count, void *sect);
static int read_cluster(struct fatfs *fatfs, uint32_t addr, void *clust);
static int dent_filename(struct fat_dirent *dent, struct fat_dirent *prev, char *buf);
static struct fs_dirent *find_entry(struct fat_dir *dir, const char *name);

static uint32_t read_fat(struct fatfs *fatfs, uint32_t addr);
static int32_t next_cluster(struct fatfs *fatfs, int32_t addr);
static int32_t find_cluster(struct fatfs *fatfs, int count, int32_t clust);

/* static void dbg_printdir(struct fat_dirent *dir, int max_entries); */
static void clean_trailws(char *s);

static struct fs_operations fs_fat_ops = {
	destroy,
	open, close,

	fsize,
	seek, tell,
	read, write,

	rewinddir, readdir,

	rename, remove
};

static unsigned char sectbuf[512];
static int max_sect_once;

struct filesys *fsfat_create(int dev, uint64_t start, uint64_t size)
{
	int num_read;
	char *endp, *ptr;
	struct filesys *fs;
	struct fatfs *fatfs;
	struct fat_dir *rootdir;
	struct bparam *bpb;
	struct bparam_ext16 *bpb16;
	struct bparam_ext32 *bpb32;

	max_sect_once = ((unsigned char*)0xa0000 - low_mem_buffer) / 512;
	/* some BIOS implementations have a maximum limit of 127 sectors */
	if(max_sect_once > 127) max_sect_once = 127;

	if(read_sectors(dev, start, 1, sectbuf) == -1) {
		return 0;
	}
	bpb = (struct bparam*)sectbuf;
	bpb16 = (struct bparam_ext16*)(sectbuf + sizeof *bpb);
	bpb32 = (struct bparam_ext32*)(sectbuf + sizeof *bpb);

	if(bpb->jmp[0] != 0xeb || bpb->jmp[2] != 0x90) {
		return 0;
	}
	if(bpb->sect_bytes != 512) {
		return 0;
	}

	if(!(fatfs = malloc(sizeof *fatfs))) {
		panic("FAT: create failed to allocate memory for the fat filesystem data\n");
	}
	memset(fatfs, 0, sizeof *fatfs);
	fatfs->dev = dev < 0 ? boot_drive_number : dev;
	fatfs->start_sect = start;
	fatfs->size = bpb->num_sectors ? bpb->num_sectors : bpb->num_sectors32;
	fatfs->cluster_size = bpb->cluster_size;
	fatfs->fat_size = bpb->fat_size ? bpb->fat_size : bpb32->fat_size;
	fatfs->fat_sect = bpb->reserved_sect;
	fatfs->root_sect = fatfs->fat_sect + fatfs->fat_size * bpb->num_fats;
	fatfs->root_size = (bpb->num_dirent * sizeof(struct fat_dirent) + 511) / 512;
	fatfs->first_data_sect = bpb->reserved_sect + bpb->num_fats * fatfs->fat_size + fatfs->root_size;
	fatfs->num_data_sect = fatfs->size - (bpb->reserved_sect + bpb->num_fats * fatfs->fat_size + fatfs->root_size);
	fatfs->num_clusters = fatfs->num_data_sect / fatfs->cluster_size;

	if(fatfs->num_clusters < 4085) {
		fatfs->type = FAT12;
	} else if(fatfs->num_clusters < 65525) {
		fatfs->type = FAT16;
	} else if(fatfs->num_clusters < 268435445) {
		fatfs->type = FAT32;
	} else {
		fatfs->type = EXFAT;
	}

	switch(fatfs->type) {
	case FAT16:
		memcpy(fatfs->label, bpb16->label, sizeof bpb16->label);
		break;

	case FAT32:
	case EXFAT:
		fatfs->root_sect = bpb32->root_clust / fatfs->cluster_size;
		fatfs->root_size = 0;
		memcpy(fatfs->label, bpb32->label, sizeof bpb32->label);
		break;

	default:
		break;
	}

	endp = fatfs->label + sizeof fatfs->label - 2;
	while(endp >= fatfs->label && isspace(*endp)) {
		*endp-- = 0;
	}

	/* read the FAT */
	if(!(fatfs->fat = malloc(fatfs->fat_size * 512))) {
		panic("FAT: failed to allocate memory for the FAT (%lu bytes)\n", (unsigned long)fatfs->fat_size * 512);
	}
	ptr = fatfs->fat;
	num_read = 0;
	while(num_read < fatfs->fat_size) {
		int count = fatfs->fat_size - num_read;
		if(count > max_sect_once) count = max_sect_once;

		read_sectors(dev, fatfs->start_sect + fatfs->fat_sect + num_read, count, ptr);
		ptr += count * 512;
		num_read += count;
	}

	/* open root directory */
	if(fatfs->type == FAT32) {
		struct fat_dirent ent;
		ent.attr = ATTR_DIR;
		ent.first_cluster_low = bpb32->root_clust;
		ent.first_cluster_high = bpb32->root_clust >> 16;
		if(!(rootdir = load_dir(fatfs, &ent))) {
			panic("FAT: failed to load FAT32 root directory\n");
		}

	} else {
		if(!(rootdir = malloc(sizeof *rootdir))) {
			panic("FAT: failed to allocate root directory structure\n");
		}
		rootdir->fatfs = fatfs;

		rootdir->max_nent = fatfs->root_size * 512 / sizeof(struct fat_dirent);
		if(!(rootdir->ent = malloc(fatfs->root_size * 512))) {
			panic("FAT: failed to allocate memory for the root directory\n");
		}
		ptr = (char*)rootdir->ent;

		num_read = 0;
		while(num_read < fatfs->root_size) {
			int count = fatfs->root_size - num_read;
			if(count > max_sect_once) count = max_sect_once;

			read_sectors(dev, fatfs->start_sect + fatfs->root_sect + num_read, count, ptr);
			ptr += count * 512;
			num_read += count;
		}

		parse_dir_entries(rootdir);
	}
	rootdir->ref = 1;
	fatfs->rootdir = rootdir;

	/* assume cluster_size is a power of two */
	fatfs->clust_mask = (fatfs->cluster_size * 512) - 1;
	fatfs->clust_shift = 0;
	while((1 << fatfs->clust_shift) < (fatfs->cluster_size * 512)) {
		fatfs->clust_shift++;
	}

	/* fill generic fs structure */
	if(!(fs = malloc(sizeof *fs))) {
		panic("FAT: create failed to allocate memory for the filesystem structure\n");
	}
	fs->type = FSTYPE_FAT;
	fs->name = fatfs->label;
	fs->fsop = &fs_fat_ops;
	fs->data = fatfs;


	printf("opened %s filesystem dev: %x, start: %lld\n", typestr[fatfs->type], fatfs->dev, start);
	if(fatfs->label[0]) {
		printf("  volume label: %s\n", fatfs->label);
	}

	return fs;
}

static void destroy(struct filesys *fs)
{
	struct fatfs *fatfs = fs->data;
	free(fatfs);
	free(fs);
}

static struct fs_node *open(struct filesys *fs, const char *path, unsigned int flags)
{
	char name[MAX_NAME];
	struct fatfs *fatfs = fs->data;
	struct fat_dir *dir, *newdir;
	struct fs_dirent *dent;
	struct fat_dirent *fatdent;
	struct fs_node *node;

	if(path[0] == '/') {
		dir = fatfs->rootdir;
		path = fs_path_skipsep((char*)path);
	} else {
		if(cwdnode->fs->type != FSTYPE_FAT) {
			return 0;
		}
		dir = cwdnode->data;
	}

	while(*path) {
		if(!dir) {
			/* we have more path components, yet the last one wasn't a dir */
			errno = ENOTDIR;
			return 0;
		}

		path = fs_path_next((char*)path, name, sizeof name);

		if(name[0] == '.' && name[1] == 0) {
			continue;
		}

		if(!(dent = find_entry(dir, name))) {
			errno = ENOENT;
			return 0;
		}
		fatdent = dent->data;

		if((fatdent->first_cluster_low | fatdent->first_cluster_high) == 0) {
			if(fatdent->attr & ATTR_DIR) {
				/* ".." entries back to the root directory seem to have a 0
				 * cluster address as a special case
				 */
				newdir = fatfs->rootdir;
			} else {
				return 0;	/* but we can't have 0-address files (right?) */
			}
		} else {
			newdir = (fatdent->attr & ATTR_DIR) ? load_dir(fatfs, fatdent) : 0;
		}
		if(dir != fatfs->rootdir && dir != cwdnode->data) {
			free_dir(dir);
		}
		dir = newdir;
	}


	if(!(node = malloc(sizeof *node))) {
		panic("FAT: open failed to allocate fs_node structure\n");
	}
	node->fs = fs;
	if(dir) {
		if(dir == fatfs->rootdir) {
			if(!(newdir = malloc(sizeof *newdir))) {
				panic("FAT: failed to allocate directory structure\n");
			}
			*newdir = *dir;
			dir = newdir;
			dir->ref = 0;
		}
		node->type = FSNODE_DIR;
		node->data = dir;
		dir->cur_ent = 0;
		dir->ref++;
	} else {
		node->type = FSNODE_FILE;
		if(!(node->data = init_file(fatfs, fatdent))) {
			panic("FAT: failed to allocate file entry structure\n");
		}
	}

	return node;
}

static void close(struct fs_node *node)
{
	switch(node->type) {
	case FSNODE_FILE:
		free_file(node->data);
		break;

	case FSNODE_DIR:
		free_dir(node->data);
		break;

	default:
		panic("FAT: close node is not a file nor a dir\n");
	}

	free(node);
}

static long fsize(struct fs_node *node)
{
	struct fat_file *file;

	if(node->type != FSNODE_FILE) {
		return -1;
	}
	file = node->data;
	return file->ent.size_bytes;
}

static int seek(struct fs_node *node, int offs, int whence)
{
	struct fatfs *fatfs;
	struct fat_file *file;
	int64_t new_pos;
	unsigned int cur_clust_idx, new_clust_idx;

	if(node->type != FSNODE_FILE) {
		return -1;
	}

	fatfs = node->fs->data;
	file = node->data;

	switch(whence) {
	case FSSEEK_SET:
		new_pos = offs;
		break;

	case FSSEEK_CUR:
		new_pos = file->cur_pos + offs;
		break;

	case FSSEEK_END:
		new_pos = file->ent.size_bytes + offs;
		break;

	default:
		return -1;
	}

	if(new_pos < 0) new_pos = 0;

	cur_clust_idx = file->cur_pos >> fatfs->clust_shift;
	new_clust_idx = new_pos >> fatfs->clust_shift;
	/* if the new position does not fall in the same cluster as the previous one
	 * re-calculate cur_clust
	 */
	if(new_clust_idx != cur_clust_idx) {
		if(new_clust_idx < cur_clust_idx) {
			file->cur_clust = find_cluster(fatfs, new_clust_idx, file->first_clust);
		} else {
			file->cur_clust = find_cluster(fatfs, new_clust_idx - cur_clust_idx, file->cur_clust);
		}
		file->buf_valid = 0;
	}
	file->cur_pos = new_pos;
	return 0;
}

static long tell(struct fs_node *node)
{
	struct fat_file *file;

	if(!node || node->type != FSNODE_FILE) {
		return -1;
	}

	file = node->data;
	return file->cur_pos;
}

static int read(struct fs_node *node, void *buf, int sz)
{
	struct fatfs *fatfs;
	struct fat_file *file;
	char *bufptr = buf;
	int num_read = 0;
	int offs, len, buf_left, rd_left;
	unsigned int cur_clust_idx, new_clust_idx;

	if(!node || !buf || sz < 0 || node->type != FSNODE_FILE) {
		return -1;
	}

	fatfs = node->fs->data;
	file = node->data;

	if(file->cur_clust < 0) {
		return 0;	/* EOF */
	}

	cur_clust_idx = file->cur_pos >> fatfs->clust_shift;

	while(num_read < sz) {
		if(!file->buf_valid) {
			read_cluster(fatfs, file->cur_clust, file->clustbuf);
			file->buf_valid = 1;
		}

		offs = file->cur_pos & fatfs->clust_mask;
		buf_left = fatfs->cluster_size * 512 - offs;
		rd_left = sz - num_read;
		len = buf_left < rd_left ? buf_left : rd_left;

		if(file->cur_pos + len > file->ent.size_bytes) {
			len = file->ent.size_bytes - file->cur_pos;
		}

		memcpy(bufptr, file->clustbuf + offs, len);
		num_read += len;
		bufptr += len;

		file->cur_pos += len;
		if(file->cur_pos >= file->ent.size_bytes) {
			file->cur_clust = -1;
			file->buf_valid = 0;
			break;	/* reached EOF */
		}

		new_clust_idx = file->cur_pos >> fatfs->clust_shift;
		if(new_clust_idx != cur_clust_idx) {
			file->buf_valid = 0;
			if((file->cur_clust = next_cluster(fatfs, file->cur_clust)) < 0) {
				break;	/* reached EOF */
			}
			cur_clust_idx = new_clust_idx;
		}
	}
	return num_read;
}

static int write(struct fs_node *node, void *buf, int sz)
{
	return -1;	/* TODO */
}

static int rewinddir(struct fs_node *node)
{
	struct fat_dir *dir;

	if(node->type != FSNODE_DIR) {
		return -1;
	}

	dir = node->data;
	dir->cur_ent = 0;
	return 0;
}

static struct fs_dirent *readdir(struct fs_node *node)
{
	struct fat_dir *dir;

	if(node->type != FSNODE_DIR) {
		return 0;
	}

	dir = node->data;
	if(dir->cur_ent >= dir->fsent_size) {
		return 0;
	}

	return dir->fsent + dir->cur_ent++;
}

static int rename(struct fs_node *node, const char *name)
{
	return -1;	/* TODO */
}

static int remove(struct fs_node *node)
{
	errno = EPERM;
	return -1;	/* TODO */
}

static struct fat_dir *load_dir(struct fatfs *fs, struct fat_dirent *dent)
{
	int32_t addr;
	struct fat_dir *dir;
	char *buf = 0;
	int bufsz = 0;

	if(!(dent->attr & ATTR_DIR)) return 0;

	addr = dent->first_cluster_low;
	if(fs->type >= FAT32) {
		addr |= (uint32_t)dent->first_cluster_high << 16;
	}

	do {
		int prevsz = bufsz;
		bufsz += fs->cluster_size * 512;
		if(!(buf = realloc(buf, bufsz))) {
			panic("FAT: failed to allocate cluster buffer (%d bytes)\n", bufsz);
		}

		if(read_cluster(fs, addr, buf + prevsz) == -1) {
			printf("load_dir: failed to read cluster: %lu\n", (unsigned long)addr);
			free(buf);
			return 0;
		}
	} while((addr = next_cluster(fs, addr)) >= 0);

	if(!(dir = malloc(sizeof *dir))) {
		panic("FAT: failed to allocate directory structure\n");
	}
	dir->fatfs = fs;
	dir->ent = (struct fat_dirent*)buf;
	dir->max_nent = bufsz / sizeof *dir->ent;
	dir->cur_ent = 0;
	dir->ref = 0;

	parse_dir_entries(dir);
	return dir;
}

static void parse_dir_entries(struct fat_dir *dir)
{
	int i;
	struct fat_dirent *dent, *prev_dent;
	struct fs_dirent *eptr;
	char entname[MAX_NAME];

	/* create an fs_dirent array with one element for each actual entry
	 * (disregarding volume labels, and LFN entries).
	 */
	if(!(dir->fsent = malloc(dir->max_nent * sizeof *dir->fsent))) {
		panic("FAT: failed to allocate dirent array\n");
	}
	eptr = dir->fsent;
	dent = dir->ent;
	prev_dent = dent - 1;

	for(i=0; i<dir->max_nent; i++) {
		if(DENT_IS_NULL(dent)) break;

		if(!DENT_IS_UNUSED(dent) && dent->attr != ATTR_VOLID && dent->attr != ATTR_LFN) {
			if(dent_filename(dent, prev_dent, entname) > 0) {
				if(!(eptr->name = malloc(strlen(entname) + 1))) {
					panic("FAT: failed to allocate dirent name\n");
				}
				strcpy(eptr->name, entname);
				eptr->data = dent;
				eptr->type = (dent->attr & ATTR_DIR) ? FSNODE_DIR : FSNODE_FILE;
				eptr->fsize = dent->size_bytes;
				eptr++;
			}
		}
		if(dent->attr != ATTR_LFN) {
			prev_dent = dent;
		}
		dent++;
	}
	dir->fsent_size = eptr - dir->fsent;
	dir->cur_ent = 0;
}

static void free_dir(struct fat_dir *dir)
{
	int i;
	struct fat_dir *root = dir->fatfs->rootdir;

	if(dir) {
		if(--dir->ref > 0) return;

		if(dir->ent != root->ent) {
			free(dir->ent);
			if(dir->fsent) {
				for(i=0; i<dir->fsent_size; i++) {
					free(dir->fsent[i].name);
				}
				free(dir->fsent);
			}
		}
		free(dir);
	}
}

static struct fat_file *init_file(struct fatfs *fatfs, struct fat_dirent *dent)
{
	struct fat_file *file;

	if(!(file = calloc(1, sizeof *file))) {
		panic("FAT: failed to allocate file structure\n");
	}
	if(!(file->clustbuf = malloc(fatfs->cluster_size * 512))) {
		panic("FAT: failed to allocate file cluster buffer\n");
	}
	file->ent = *dent;
	file->first_clust = dent->first_cluster_low | ((int32_t)dent->first_cluster_high << 16);
	file->cur_clust = file->first_clust;
	return file;
}

static void free_file(struct fat_file *file)
{
	if(file) {
		free(file->clustbuf);
		free(file);
	}
}

static int read_sectors(int dev, uint64_t sidx, int count, void *sect)
{
	if(dev == -1 || dev == boot_drive_number) {
		if(bdev_read_range(sidx, count, sect) == -1) {
			return -1;
		}
		return 0;
	}

	printf("BUG: reading sectors from drives other than the boot drive not implemented yet\n");
	return -1;
}

static int read_cluster(struct fatfs *fatfs, uint32_t addr, void *clust)
{
	char *ptr = clust;
	uint64_t saddr = (uint64_t)(addr - 2) * fatfs->cluster_size + fatfs->first_data_sect + fatfs->start_sect;

	if(read_sectors(fatfs->dev, saddr, fatfs->cluster_size, ptr) == -1) {
		return -1;
	}
	return 0;
}

static int dent_filename(struct fat_dirent *dent, struct fat_dirent *prev, char *buf)
{
	int len = 0;
	char *ptr = buf;
	struct fat_lfnent *lfn = (struct fat_lfnent*)(dent - 1);

	if(lfn > (struct fat_lfnent*)prev && lfn->attr == ATTR_LFN) {
		/* found a long filename entry, use that */
		do {
			uint16_t ustr[14], *uptr = ustr;
			memcpy(uptr, lfn->part1, sizeof lfn->part1);
			uptr += sizeof lfn->part1 / sizeof *lfn->part1;
			memcpy(uptr, lfn->part2, sizeof lfn->part2);
			uptr += sizeof lfn->part2 / sizeof *lfn->part2;
			memcpy(uptr, lfn->part3, sizeof lfn->part3);
			ustr[13] = 0;

			uptr = ustr;
			while(*uptr) {
				*ptr++ = *(char*)uptr++;
				len++;
			}
			*ptr = 0;

			if(uptr < ustr + 13 || (lfn->seq & 0xf0) == 0x40) break;
			lfn -= 1;
		} while(lfn > (struct fat_lfnent*)prev && lfn->attr == ATTR_LFN);

	} else {
		/* regular 8.3 filename */
		memcpy(buf, dent->name, 8);
		buf[8] = 0;
		clean_trailws(buf);
		if(!buf[0]) return 0;

		if(dent->name[8] && dent->name[8] != ' ') {
			ptr = buf + strlen(buf);
			*ptr++ = '.';
			memcpy(ptr, dent->name + 8, 3);
			ptr[3] = 0;
			clean_trailws(ptr);
		}

		len = strlen(buf);
	}
	return len;
}

static struct fs_dirent *find_entry(struct fat_dir *dir, const char *name)
{
	int i;
	struct fs_dirent *dent = dir->fsent;

	for(i=0; i<dir->fsent_size; i++) {
		if(strcasecmp(dent->name, name) == 0) {
			return dent;
		}
		dent++;
	}
	return 0;
}

static uint32_t read_fat(struct fatfs *fatfs, uint32_t addr)
{
	uint32_t res = 0xffffffff;

	switch(fatfs->type) {
	case FAT12:
		{
			uint32_t idx = addr + addr / 2;
			res = ((uint16_t*)fatfs->fat)[idx];

			if(idx & 1) {
				res >>= 4;		/* odd entries end up on the high 12 bits */
			} else {
				res &= 0xfff;	/* even entries end up on the low 12 bits */
			}
		}
		break;

	case FAT16:
		res = ((uint16_t*)fatfs->fat)[addr];
		break;

	case FAT32:
	case EXFAT:
		res = ((uint32_t*)fatfs->fat)[addr];
		break;

	default:
		break;
	}

	return res;
}

static int32_t next_cluster(struct fatfs *fatfs, int32_t addr)
{
	uint32_t fatval = read_fat(fatfs, addr);

	if(fatval == 0) return -1;

	switch(fatfs->type) {
	case FAT12:
		if(fatval >= 0xff8) return -1;
		break;

	case FAT16:
		if(fatval >= 0xfff8) return -1;
		break;

	case FAT32:
	case EXFAT:	/* XXX ? */
		if(fatval >= 0xffffff8) return -1;
		break;

	default:
		break;
	}

	return fatval;
}

static int32_t find_cluster(struct fatfs *fatfs, int count, int32_t clust)
{
	while(count-- > 0 && (clust = next_cluster(fatfs, clust)) >= 0);
	return clust;
}

/*
static void dbg_printdir(struct fat_dirent *dir, int max_entries)
{
	char name[MAX_NAME];
	struct fat_dirent *prev = dir - 1;
	struct fat_dirent *end = max_entries > 0 ? dir + max_entries : 0;

	while(!DENT_IS_NULL(dir) && (!end || dir < end)) {
		if(!DENT_IS_UNUSED(dir) && dir->attr != ATTR_VOLID && dir->attr != ATTR_LFN) {
			if(dent_filename(dir, prev, name) > 0) {
				printf("%s%c\n", name, (dir->attr & ATTR_DIR) ? '/' : ' ');
			}
		}
		if(dir->attr != ATTR_LFN) {
			prev = dir;
		}
		dir++;
	}
}
*/

static void clean_trailws(char *s)
{
	char *p;

	if(!s || !*s) return;

	p = s + strlen(s) - 1;
	while(p >= s && isspace(*p)) p--;
	p[1] = 0;
}
