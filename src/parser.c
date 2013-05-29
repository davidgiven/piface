/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"

#define MAX_WORDS 32
static char* argv[MAX_WORDS];
static const char* parseerror;

static void help_cb(int argc, const char* argv[]);

static const struct command help_cmd =
{
	"help",
	"gets help on available commands",

	"On its own, lists the available PiFace commands; when used with an\n"
	"argument, shows more detailed information on one particular command.",

	help_cb
};

static const struct command* commands[] =
{
	&help_cmd,
};
#define NUM_COMMANDS sizeof(commands)/sizeof(*commands)

static void parse_buffer(char* buffer)
{
	int word = 0;
	char* inp = buffer;
	char* outp = buffer;
	int c;

	goto waitforword;

waitforword: /* skip whitespace leading up to new word */
	while (isspace(*inp))
		inp++;
	goto startword;

startword: /* start of new word; is this the end of string? */
	if (!*inp)
		goto endofstring;
	argv[word] = outp;
	goto nextwordchar;

nextwordchar: /* add a char to the current word */
	c = *inp++;
	switch (c)
	{
		case '\0': goto endofword;
		case '\'': goto singlequote;
		case '\"': goto doublequote;
	}
	if (isspace(c))
		goto endofword;
	*outp++ = c;
	goto nextwordchar;

endofword: /* commit the current word and move to the next one */
	word++;
	if (word == MAX_WORDS)
	{
		parseerror = "Too many words in command line";
		goto error;
	}
	*outp++ = '\0';
	goto waitforword;

singlequote: /* single quoted text */
	c = *inp++;
	switch (c)
	{
		case '\'': goto nextwordchar;
		case '\0':
			parseerror = "Unterminated single quote";
			goto error;
	}
	*outp++ = c;
	goto singlequote;

doublequote: /* double quoted text */
	c = *inp++;
	switch (c)
	{
		case '\"': goto nextwordchar;
		case '\0':
			parseerror = "Unterminated double quote";
			goto error;
	}
	*outp++ = c;
	goto doublequote;

endofstring: /* parsing succeeded */
	argv[word] = '\0';
	parseerror = NULL;
	return;

error: /* parsing failed */
	argv[0] = '\0';
	return;
}

static void help_cb(int argc, const char* argv[])
{
	printf("Help!\n");
}

void execute_command(char* buffer)
{
	int i;
	int argc;

	parse_buffer(buffer);
	if (parseerror)
	{
		printf("Parse error: %s\n", parseerror);
		return;
	}

	/* Count commands. */

	{
		const char** p = (const char**) argv;
		argc = 0;
		while (*p++)
			argc++;
	}

	/* Empty command lines are noops. */

	if (argc == 0)
		return;

	/* Look for the command and run it if it exists. */

	for (i=0; i<NUM_COMMANDS; i++)
	{
		if (strcmp(commands[i]->name, argv[0]) == 0)
		{
			commands[i]->callback(argc, (const char**) argv);
			return;
		}
	}

	printf("Command not recognised (try help).\n");
}
