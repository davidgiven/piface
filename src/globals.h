/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>

#if defined TARGET_PI
	#include <pi.h>
#endif

/* Error reporting */

extern char* error;
extern void clearError(void);
extern void setError(const char* msg, ...);

/* User commands */

struct command
{
	const char* name;
	const char* description;
	const char* help;
	void (*callback)(int argc, const char* argv[]);
};

extern const struct command sx_cmd;
extern const struct command rx_cmd;

/* Command line parser (do not use reentrantly) */

extern void execute_command(char* cmd);

/* VFS declarations */

struct vfs
{
	const char* name;
	struct file* (*open)(const char* path, int flags);
};

struct filecbs
{
	void (*close)(void* backend);
	uint32_t (*read)(void* backend,
		uint32_t offset, void* buffer, uint32_t length);
	uint32_t (*write)(void* backend,
		uint32_t offset, void* buffer, uint32_t length);
};

struct file
{
	void* backend;
	struct filecbs* cb;
};

extern struct file* vfs_open(const char* path);
extern void vfs_close(struct file* fp);
extern uint32_t vfs_read(struct file* fp,
	uint32_t offset, void* buffer, uint32_t length);
extern uint32_t vfs_write(struct file* fp,
	uint32_t offset, void* buffer, uint32_t length);

/* MMC interface */

extern void mmc_init(void);

#endif
