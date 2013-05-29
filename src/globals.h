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
#include <ctype.h>

#if defined TARGET_PI
	#include <pi.h>
#endif

/* User commands */

struct command
{
	const char* name;
	const char* description;
	const char* help;
	void (*callback)(int argc, const char* argv[]);
};

/* Command line parser (do not use reentrantly) */

extern void execute_command(char* cmd);

/* MMC interface */

extern void mmc_init(void);

#endif
