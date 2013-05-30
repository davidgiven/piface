/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"

static void sx_cb(int argc, const char* argv[]);
static void rx_cb(int argc, const char* argv[]);

const struct command sx_cmd =
{
	"sx",
	"sends a file by XMODEM",

	"Syntax:\n"
	"  sx <filename>\n"
	"Attempts to transmit the file via the console by XMODEM.",

	sx_cb
};

const struct command rx_cmd =
{
	"rx",
	"receives a file by XMODEM",

	"Syntax:\n"
	"  rx <filename>\n"
	"Attempts to receive a file via the console by XMODEM.",

	rx_cb
};

static void sx_cb(int argc, const char* argv[])
{
	struct file* fp;

	if (argc != 2)
	{
		setError("syntax: sx <filename>");
		return;
	}

	fp = vfs_open(argv[1], O_RDONLY);
	if (!fp)
		return;
}

static void rx_cb(int argc, const char* argv[])
{
	setError("unimplemented");
}

