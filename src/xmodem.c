/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"
#include <termios.h>

static void xmodem_send(struct file* fp, int len)
{
	uint8_t block;
	uint32_t offset;
	uint32_t thisblocklen;
	int crc16;
	uint16_t crc;
	uint8_t* buffer;
	uint8_t c;

	printf("Give your local XMODEM receive command now.\n");
	fflush(stdout);
	newlines_off();

	buffer = malloc(1024);

	block = 1;
	offset = 0;
	crc16 = 0;
	if (len >= 1024)
		thisblocklen = 1024;
	else
		thisblocklen = 128;
	for (;;)
	{
		int i;
		fflush(stdout);
		c = getchar();

        switch (c)
        {
            case 'C': /* enable CRC-16 mode */
                crc16 = 1;
                /* fall through */
			case 21: /* NAK; repeat current block */
                break;

			case 6: /* ACK; advance to next block */
				block++;
				offset += thisblocklen;
				if (offset >= len)
					goto eof;

				thisblocklen = len - offset;
				if (thisblocklen >= 1024)
					thisblocklen = 1024;
				else
					thisblocklen = 128;
				break;

			default: /* ignore everything else */
				continue;
        }

		/* Read the block from the file. */

		i = vfs_read(fp, offset, buffer, thisblocklen);
		if (i < thisblocklen)
			memset(buffer+i, 26, thisblocklen-i); /* SUB */

		/* Calculate CRC. */

		crc = 0;
		for (i=0; i<thisblocklen; i++)
		{
			c = buffer[i];

			if (crc16)
			{
				int j;

				crc ^= c<<8;
				for (j=0; j<8; j++)
				{
					crc <<= 1;
					if (crc & 0x10000)
						crc ^= 0x1021;
				}
			}
			else
				crc += c;
		}

		/* Write out the packet. */

        if (thisblocklen == 128)
            putchar(1); /* SOH */
		else
			putchar(2); /* STX */
		putchar(block);
		putchar(~block);

		fwrite(buffer, 1, thisblocklen, stdout);

		if (crc16)
			putchar(crc >> 8);
		putchar(crc);
	}
eof:

	putchar(4); /* EOT */

	/* Wait for ACK (we have to block here or the receiver will barf). */
	fflush(stdout);
	c = getchar();

	free(buffer);
	fflush(stdout);
	newlines_on();
	printf("File transmission complete.\n");
}

static void send_cb(int argc, const char* argv[])
{
	struct file* fp;
	uint32_t len;

	if (argc != 2)
	{
		setError("syntax: send <filename>");
		return;
	}

	fp = vfs_open(argv[1], O_RDONLY);
	if (!fp)
		return;

	vfs_info(fp, NULL, &len);
	if (len & 0x7f)
		printf("Warning: file is not a multiple of 128 bytes, padding will be added\n");
	xmodem_send(fp, len);
	vfs_close(fp);
}

static void recv_cb(int argc, const char* argv[])
{
	setError("unimplemented");
}

const struct command send_cmd =
{
	"send",
	"sends a file by XMODEM",

	"Syntax:\n"
	"  send <filename>\n"
	"Attempts to transmit the file via the console by XMODEM.",

	send_cb
};

const struct command recv_cmd =
{
	"recv",
	"receives a file by XMODEM",

	"Syntax:\n"
	"  recv <filename>\n"
	"Attempts to receive a file via the console by XMODEM.",

	recv_cb
};


