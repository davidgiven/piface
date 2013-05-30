/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"

#if 0
static const struct vfs* vfs[] =
{
};
#endif
#define NUM_VFS sizeof(vfs)/sizeof(*vfs)

struct file* vfs_open(const char* path)
{
	const char* e = strchr(path, ':');
	int len;

	if (!e)
	{
		setError("malformed path (no VFS specifier)");
		return NULL;
	}

	len = e-path;

	printf("vfs=<%.*s>\n", len, path);
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
