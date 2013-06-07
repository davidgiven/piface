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
	"executes code at a particular address",

	"Syntax:\n"
	"  go <address>\n"
	"Calls a machine code routine at <address> (a hex number). If the\n"
	"routine does not corrupt piface's workspace, then returning will\n"
	"resume.",

	go_cb
};

static void poke_cb(int argc, const char* argv[])
{
	unsigned size;
	uint32_t addr;
	char* p;
	int i;

	if (argc <= 3)
	{
		setError("syntax: poke <size> <address> <values...>");
		return;
	}

	if (strcmp(argv[1], "quad") == 0)
		size = 4;
	else if (strcmp(argv[1], "word") == 0)
		size = 2;
	else if (strcmp(argv[1], "byte") == 0)
		size = 1;
	else
	{
		setError("use 'quad', 'word' or 'byte' for the size");
		return;
	}

    addr = strtoul(argv[2], &p, 16);
    if (*p)
    {
        setError("unable to parse address (don't put 0x on the front)");
        return;
    }

	addr = (uint32_t) pi_phys_to_user((void*) addr);
	for (i=3; i<argc; i++)
	{
		uint32_t value = strtoul(argv[i], &p, 16);
		if (*p)
		{
			setError("unable to parse value %d (don't put 0x on the front)", i);
			return;
		}

		switch (size)
		{
			case 4:
				*(uint32_t*)addr = value;
				addr += 4;
				break;

			case 2:
				*(uint16_t*)addr = value;
				addr += 2;
				break;

			case 1:
				*(uint8_t*)addr = value;
				addr += 1;
				break;
		}
	}
}

const struct command poke_cmd =
{
	"poke",
	"modifies data in memory",

	"Syntax:\n"
	"  poke <size> <address> <values...>\n"
	"<size> is either 'quad', 'word' or 'byte'. <address> is the address\n"
	"(in hex, without a leading 0x). <values...> is a whitespace-terminated\n"
	"list of values (formatted likewise) which will be written to consecutive\n"
	"memory locations.",

	poke_cb
};
