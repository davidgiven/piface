/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"
#include <termios.h>

static struct termios oldtermios;
static char* buffer = NULL;
static int bufferlen = 0;
static int width;
static int xpos;

static void deinit_console(void)
{
	tcsetattr(0, TCSAFLUSH, &oldtermios);
}

void init_console(void)
{
	struct termios newtermios;
	tcgetattr(0, &oldtermios);
	newtermios = oldtermios;
	newtermios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    newtermios.c_oflag &= ~(OPOST);
    newtermios.c_cflag |= (CS8);
    newtermios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
	tcsetattr(0, TCSAFLUSH, &newtermios);
	atexit(deinit_console);

	setvbuf(stdin, NULL, _IONBF, 0);
	setvbuf(stdout, NULL, _IOFBF, 1030);
}

void newlines_on(void)
{
	struct termios t;
	tcgetattr(0, &t);
    t.c_oflag |= (INLCR | OPOST);
	tcsetattr(0, TCSAFLUSH, &t);
}

void newlines_off(void)
{
	struct termios t;
	tcgetattr(0, &t);
    t.c_oflag &= ~(INLCR | OPOST);
	tcsetattr(0, TCSAFLUSH, &t);
}

static void extendbuffer(int size)
{
	if (size > bufferlen)
	{
		bufferlen *= 2;
		if (bufferlen == 0)
			bufferlen = 16;
		buffer = realloc(buffer, bufferlen);
	}
}

static void getcursorpos(int* x, int* y)
{
	char buffer[16];
	int i = 0;

	fwrite("\033[6n", 1, 4, stdout);
	fflush(stdout);

	while (i<15)
	{
		int c = getchar();
		buffer[i++] = c;

		if (c == 'R')
			break;
	}
	buffer[i] = '\0';

	i = sscanf(buffer, "\033[%d;%dR", y, x);
	if (i != 2)
		*x = *y = 0;
}

static void backspaces(int n)
{
	while (n--)
	{
		if (xpos == 0)
		{
			printf("\033[A\033[%dC", width-1);
			xpos = width-1;
		}
		else
		{
			putchar(8);
			xpos--;
		}
	}
}

static void outchar(int c)
{
	putchar(c);
	xpos++;
	if (xpos == width)
	{
		printf("\n\r");
		xpos = 0;
	}
}

static void outstring(const char* s, int len)
{
	while (len--)
		outchar(*s++);
}

char* readline(void)
{
	int cursor = 0;
	int oldcursor = 0;
	int stringlen = 0;
	int oldstringlen = 0;
	int i;
	char c;

#if defined TARGET_TESTBED
	//sleep(10);
#endif

	/* Find out where the cursor is and how big the terminal is. */

	{
		int ypos, height;
		getcursorpos(&xpos, &ypos);
		printf("\033[999;999f");
		getcursorpos(&width, &height);
		width++; /* because the value read above is inclusive */
		printf("\033[%d;%df", ypos, xpos);
	}

	printf("\033[7l"); /* disable line wrap */

	for (;;)
	{
		backspaces(oldcursor);
		printf("\033[K\033[J");
		outstring(buffer, stringlen);
		backspaces(stringlen-cursor);

		oldstringlen = stringlen;
		oldcursor = cursor;

		fflush(stdout);
		fread(&c, 1, 1, stdin);
		switch (c)
		{
			case '\n':
			case '\r':
				goto eos;

			case 1: /* ^A */
				cursor = 0;
				break;

			case 5: /* ^E */
				cursor = stringlen;
				break;

			case 3: /* ^C */
			case 21: /* ^U */
				cursor = stringlen = 0;
				break;

			case 2: /* ^B */
				if (cursor > 0)
					cursor--;
				break;

			case 6: /* ^F */
				if (cursor < stringlen)
					cursor++;
				break;

			case 4: /* ^D */
				if (cursor < stringlen)
				{
					memmove(buffer+cursor, buffer+cursor+1, stringlen-cursor-1);
					stringlen--;
				}
				break;

			case 8: /* ^H */
			case 127: /* DEL */
				if (cursor > 0)
				{
					memmove(buffer+cursor-1, buffer+cursor, stringlen-cursor);
					cursor--;
					stringlen--;
				}
				break;

			default:
				if ((c >= 32) && (c <= 126))
				{
					extendbuffer(stringlen+1);

					memmove(buffer+cursor+1, buffer+cursor, stringlen-cursor);
					stringlen++;

					buffer[cursor] = c;
					cursor++;
				}
				break;
		}
	}
eos:

	extendbuffer(stringlen+1);
	buffer[stringlen] = '\0';
	printf("\n");
	return buffer;
}
