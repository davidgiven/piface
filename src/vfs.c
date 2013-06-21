/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"

static const struct vfs* vfs[] =
{
	&vfs_mem,
#if defined TARGET_TESTBED
	&vfs_host,
#endif
#if defined TARGET_PI
	&vfs_sd,
#endif
};
#define NUM_VFS sizeof(vfs)/sizeof(*vfs)

static const struct vfs* find_vfs(const char* name, int namelen)
{
	int i;

	for (i=0; i<NUM_VFS; i++)
	{
		if ((memcmp(vfs[i]->name, name, namelen) == 0) &&
		    (vfs[i]->name[namelen] == '\0'))
			return vfs[i];
	}

	return NULL;
}

static int parse_vfs_path(const char* path, const struct vfs** fs,
	const char** sub)
{
	const char* e = strchr(path, ':');
	int len;

	if (!e)
	{
		setError("malformed path (no VFS specifier)");
		return 0;
	}

	len = e-path;
	*fs = find_vfs(path, len);
	if (!fs)
	{
		setError("unknown file system '%.*s'", len, path);
		return 0;
	}

	*sub = e+1;
	return 1;
}

struct file* vfs_open(const char* path, int flags)
{
	const struct vfs* fs;
	const char* subpath;
	void* backend;

	if (!parse_vfs_path(path, &fs, &subpath))
		return NULL;

	backend = fs->open(subpath, flags);
	if (backend)
	{
		struct file* fp = malloc(sizeof(struct file));
		fp->backend = backend;
		fp->cb = fs->callbacks;
		return fp;
	}
	return NULL;
}

void vfs_close(struct file* fp)
{
	fp->cb->close(fp->backend);
	free(fp);
}

uint32_t vfs_read(struct file* fp, uint32_t offset, void* buffer, uint32_t len)
{
	return fp->cb->read(fp->backend, offset, buffer, len);
}

uint32_t vfs_write(struct file* fp, uint32_t offset, void* buffer, uint32_t len)
{
	return fp->cb->write(fp->backend, offset, buffer, len);
}

void vfs_info(struct file* fp, uint32_t* base, uint32_t* length)
{
	uint32_t dummy;
	if (!base)
		base = &dummy;
	if (!length)
		length = &dummy;
	fp->cb->info(fp->backend, base, length);
}

void vfs_enumerate(const char* path, vfs_enumerate_f* cb)
{
	const struct vfs* fs;
	const char* subpath;

	if (!parse_vfs_path(path, &fs, &subpath))
		return;

	if (!fs->enumerate)
	{
		setError("filesystem does not support enumeration");
		return;
	}

	fs->enumerate(subpath, cb);
	return;
}


