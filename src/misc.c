/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"

static void go_cb(int argc, const char* argv[])
{
	uint32_t addr;
	char* p;

	if (argc != 2)
	{
		setError("syntax: go <address>");
		return;
	}

    addr = strtoul(argv[1], &p, 16);
    if (*p)
    {
        setError("unable to parse address (don't put 0x on the front)");
        return;
    }

    addr = (uint32_t)pi_phys_to_user((void*) addr);

    {
        typedef void func_t(void);
        func_t* cb = (func_t*)(void*) addr;
        cb();
    }
}

const struct command go_cmd =
{
	"go",
	"executes code at a particular address\n",

	"Syntax:\n"
	"  go <address>\n"
	"Calls a machine code routine at <address> (a hex number). If the\n"
	"routine does not corrupt piface's workspace, then returning will\n"
	"resume.",

	go_cb
};
