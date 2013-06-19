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
#include <errno.h>
#include <fcntl.h>
#include <termios.h>

extern char** environ;

#if defined TARGET_PI
	#include <pi.h>
#else
	#define pi_phys_to_user(x) x
	#define pi_user_to_phys(x) x
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

extern const struct command send_cmd;
extern const struct command recv_cmd;
extern const struct command dump_cmd;
extern const struct command go_cmd;
extern const struct command poke_cmd;

/* Command line parser (do not use reentrantly) */

extern void init_console(void);
extern void newlines_on(void);
extern void newlines_off(void);
extern char* readline(void);
extern void execute_command(char* cmd);

/* VFS declarations */

struct vfs
{
	const char* name;
	const struct filecbs* callbacks;
	void* (*open)(const char* path, int flags);
};

struct filecbs
{
	void (*close)(void* backend);
	uint32_t (*read)(void* backend,
		uint32_t offset, void* buffer, uint32_t length);
	uint32_t (*write)(void* backend,
		uint32_t offset, void* buffer, uint32_t length);
	void (*info)(void* backend, uint32_t* base, uint32_t* length);
};

struct file
{
	void* backend;
	const struct filecbs* cb;
};

extern struct file* vfs_open(const char* path, int flags);
extern void vfs_close(struct file* fp);
extern uint32_t vfs_read(struct file* fp,
	uint32_t offset, void* buffer, uint32_t length);
extern uint32_t vfs_write(struct file* fp,
	uint32_t offset, void* buffer, uint32_t length);
extern void vfs_info(struct file* fp,
	uint32_t* base, uint32_t* length);

extern const struct vfs vfs_host;
extern const struct vfs vfs_mem;

/* MMC interface */

extern void mmc_init(void);

/* Utilities */

extern void millisleep(uint32_t ms);

#endif
