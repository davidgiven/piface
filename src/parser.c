/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"

#define MAX_WORDS 32
static char* argv[MAX_WORDS];

static void help_cb(int argc, const char* argv[]);
static void set_cb(int argc, const char* argv[]);

static const struct command help_cmd =
{
	"help",
	"gets help on available commands",

	"On its own, lists the available PiFace commands; when used with an\n"
	"argument, shows more detailed information on one particular command.",

	help_cb
};

static const struct command set_cmd =
{
	"set",
	"sets, lists or unsets an environment variable",

	"To set one or more variables:\n"
	"  set NAME1=VALUE1 NAME2=VALUE2...\n"
	"\n"
	"To unset one or more variables:\n"
	"  set NAME1= NAME2=...\n"
	"\n"
	"To list all variables:\n"
	"  set\n"
	"\n"
	"(To set a variable to a value containing spaces, quote the argument.)",

	set_cb
};

static const struct command* commands[] =
{
	&help_cmd,
	&set_cmd,
	&sx_cmd,
	&rx_cmd,
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
		setError("too many words in command line");
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
			setError("unterminated single quote");
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
			setError("unterminated double quote");
			goto error;
	}
	*outp++ = c;
	goto doublequote;

endofstring: /* parsing succeeded */
	argv[word] = '\0';
	return;

error: /* parsing failed */
	argv[0] = '\0';
	return;
}

static const struct command* find_command(const char* name)
{
	int i;

	for (i=0; i<NUM_COMMANDS; i++)
	{
		if (strcmp(commands[i]->name, name) == 0)
			return commands[i];
	}

	return NULL;
}

static void help_cb(int argc, const char* argv[])
{
	if (argc == 1)
	{
		int i;

		printf("Available commands:\n\n");
		for (i=0; i<NUM_COMMANDS; i++)
			printf("% 10s  %s\n", commands[i]->name, commands[i]->description);
		printf("\nTry 'help <command>' for more information.\n");
	}
	else if (argc == 2)
	{
		const struct command* cmd = find_command(argv[1]);
		if (cmd)
			printf("%s\n", cmd->help);
		else
			setError("unknown command '%s'", argv[1]);
	}
	else
		setError("'help' only understands one parameter");
}

static void set_cb(int argc, const char* argv[])
{
	if (argc == 1)
	{
		char** p = environ;
		if (p)
		{
			while (*p)
			{
				printf("%s\n", *p);
				p++;
			}
		}
		return;
	}

	{
		const char** p = &argv[1];
		while (*p)
		{
			char* key = strdup(*p);
			char* value;

			strtok(key, "=");
			value = strtok(NULL, "=");

			if (!value || !*value)
				unsetenv(key);
			else
				setenv(key, value, 1);

			free(key);
			p++;
		}
	}
}

void execute_command(char* buffer)
{
	int argc;

	parse_buffer(buffer);
	if (error)
		return;

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

	{
		const struct command* cmd = find_command(argv[0]);
		if (cmd)
			cmd->callback(argc, (const char**) argv);
		else
			setError("Command '%s' not recognised (try 'help').");
	}
}
