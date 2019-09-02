#ifndef FILE_H_
#define FILE_H_

#include <stdio.h>
#include <errno.h>
#include "fs.h"
#include "panic.h"

enum {
	MODE_READ = 1,
	MODE_WRITE = 2,
	MODE_APPEND = 4,
	MODE_CREATE = 8,
	MODE_TRUNCATE = 16
};

enum {
	STATUS_EOF	= 1,
	STATUS_ERR	= 2
};

struct FILE {
	unsigned int mode;
	unsigned int status;
	struct fs_node *fsn;
};

FILE *fopen(const char *path, const char *mode)
{
	FILE *fp;
	struct fs_node *node;
	unsigned int mflags = 0;

	while(*mode) {
		int c = *mode++;
		switch(c) {
		case 'r':
			mflags |= MODE_READ;
			if(*mode == '+') {
				mflags |= MODE_WRITE;
				mode++;
			}
			break;
		case 'w':
			mflags |= MODE_WRITE | MODE_TRUNCATE;
			if(*mode == '+') {
				mflags |= MODE_READ | MODE_CREATE;
				mode++;
			}
			break;
		case 'a':
			mflags |= MODE_WRITE | MODE_APPEND;
			if(*mode == '+') {
				mflags |= MODE_READ | MODE_CREATE;
				mode++;
			}
			break;
		case 'b':
			break;
		default:
			errno = EINVAL;
			return 0;
		}
	}

	if(!(node = fs_open(path, 0))) {
		/* TODO: create */
		errno = ENOENT;	/* TODO */
		return 0;
	}
	if(node->type != FSNODE_FILE) {
		errno = EISDIR;
		return 0;
	}

	if(!(fp = malloc(sizeof *fp))) {
		errno = ENOMEM;
		return 0;
	}
	fp->fsn = node;
	fp->mode = mflags;
	fp->status = 0;

	return fp;
}

int fclose(FILE *fp)
{
	if(!fp) {
		errno = EINVAL;
		return -1;
	}

	fs_close(fp->fsn);
	free(fp);
	return 0;
}

long filesize(FILE *fp)
{
	return fs_filesize(fp->fsn);
}

int fseek(FILE *fp, long offset, int from)
{
	if(!fp) {
		errno = EINVAL;
		return -1;
	}
	if(from < 0 || from > 2) {
		errno = EINVAL;
		return -1;
	}

	fs_seek(fp->fsn, offset, from);
	fp->status &= ~STATUS_EOF;
	return 0;
}

void rewind(FILE *fp)
{
	fseek(fp, 0, SEEK_SET);
}

long ftell(FILE *fp)
{
	if(!fp) {
		errno = EINVAL;
		return -1;
	}
	return fs_tell(fp->fsn);
}

size_t fread(void *buf, size_t size, size_t count, FILE *fp)
{
	int res;
	if(!fp) return 0;
	if((res = fs_read(fp->fsn, buf, size * count)) == -1) {
		fp->status |= STATUS_EOF;
		return 0;
	}
	return res / size;
}

size_t fwrite(const void *buf, size_t size, size_t count, FILE *fp)
{
	int res;
	if(!fp) return 0;
	if(!(fp->mode & MODE_WRITE)) {
		fp->status |= STATUS_ERR;
		return 0;
	}
	if((res = fs_write(fp->fsn, (void*)buf, size * count)) == -1) {
		return 0;
	}
	return res / size;
}

int fgetc(FILE *fp)
{
	unsigned char c;
	if(fread(&c, 1, 1, fp) < 1) {
		return -1;
	}
	return c;
}

char *fgets(char *buf, int size, FILE *fp)
{
	int c;
	char *s = buf;

	while(--size > 0 && (c = fgetc(fp)) >= 0) {
		*s++ = c;
		if(c == '\n') break;
	}
	*s = 0;
	return s > buf ? buf : 0;
}

int fputc(int c, FILE *fp)
{
	if(fp == stdout || fp == stderr) {
		return putchar(c);
	}

	panic("fputc on anything other than stdout/stderr not implemented yet\n");
	return -1;
}

int fflush(FILE *fp)
{
	if(fp == stdout || fp == stderr) {
		return 0;	/* do nothing */
	}

	panic("fflush on anything other than stdout/stderr not implemented yet\n");
	return -1;
}

int feof(FILE *fp)
{
	return (fp->status & STATUS_EOF) != 0;
}

int ferror(FILE *fp)
{
	return (fp->status & STATUS_ERR) != 0;
}

void clearerr(FILE *fp)
{
	fp->status = 0;
}

#endif	/* FILE_H_ */
