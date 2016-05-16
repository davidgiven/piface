/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"
#ifdef __GNUC__
#include <sys/time.h>
#endif
#include <termios.h>

static int crc16;
static uint16_t crc;

static void update_crc(const uint8_t* data, unsigned len)
{
	unsigned i;

	for (i=0; i<len; i++)
	{
		uint8_t c = data[i];

		if (crc16)
		{
			int j;

			crc ^= c<<8;
			for (j=0; j<8; j++)
			{
				if (crc & 0x8000)
                    crc = (crc << 1) ^ 0x1021;
                else
                    crc = (crc << 1);
			}
		}
		else
			crc += c;
	}
}

static int poll_stdin(void)
{
	struct timeval t = {1, 0};
	fd_set rds, wrs, exs;
	FD_ZERO(&rds);
	FD_ZERO(&wrs);
	FD_ZERO(&exs);
	FD_SET(0, &rds);

	fflush(stdout);
	return select(1, &rds, &wrs, &exs, &t);
}

static void xmodem_send(struct file* fp, int len)
{
	uint8_t block;
	uint32_t offset;
	uint32_t thisblocklen;
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
		update_crc(buffer, thisblocklen);

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
	millisleep(1000);
	printf("File transmission complete.\n");
}

static void xmodem_recv(struct file* fp)
{
	uint8_t block, nextblock;
	uint32_t offset;
	uint32_t nextoffset;
	uint8_t* buffer;
	uint8_t c;
	uint32_t thisblocksize;
	uint16_t blockcrc;
	int command;

	printf("Give your local XMODEM send command now.\n");
	fflush(stdout);
	newlines_off();

	offset = nextoffset = 0;
	block = 1;
	command = 'C';

	buffer = malloc(1024+4); /* maximum size for a packet */

	for (;;)
	{
		/* Send command and wait for response. */

		putchar(command);
		if (!poll_stdin())
		{
			/* Timeout. Go round and send the command again. */
			continue;
		}

		/* Read response. */

		c = getchar();
		switch (c)
		{
			case 1: /* SOH */
				thisblocksize = 128;
				break;

			case 2: /* STX */
				thisblocksize = 1024;
				break;

			case 4: /* EOT */
				goto eot;

			default:
			{
				/* Mangled packet! There's not really much we can do here.
				 * Wait for data to stop and send a NAK. */

				while (poll_stdin())
					getchar();

				if (offset == 0)
					command = 'C';
				else
					command = 21; /* NAK */
				continue;
			}
		}

		/* Okay, we are about to receive a hopefully valid packet of length
		 * thisblocksize. */

		{
			int len = thisblocksize + 4;
			int count = 0;

			while (count < len)
			{
				int i = fread(buffer+count, 1, len, stdin);
				len -= i;
				count += i;
			}
		}

		/* Check the block number. */

		crc16 = 1;
		crc = 0;
		update_crc(buffer+2, thisblocksize);

		nextblock = block + 1; /* ensure wrapping occurs */
		blockcrc = (buffer[2+thisblocksize+0]<<8) | buffer[2+thisblocksize+1];
		if ((buffer[0] == (~buffer[1] & 0xff))
		    && ((buffer[0] == block) || (buffer[0] == nextblock))
		    && (blockcrc == crc)
		    )
		{
			/* Valid packet! Write it to disk, first checking to see whether
			 * this is a new block or not. */

			if (buffer[0] == nextblock)
			{
				offset = nextoffset;
				block = nextblock;
			}

			vfs_write(fp, offset, buffer+2, thisblocksize);
			nextoffset = offset + thisblocksize;

			command = 6; /* ACK */
		}
		else
		{
			/* Invalid packet --- request resend. */
			command = 21; /* NAK */
		}
	}

eot:
	putchar(6); /* ACK */
	fflush(stdout);

	free(buffer);

	newlines_on();
	millisleep(1000);
	printf("File reception complete.\n");
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
	struct file* fp;

	if (argc != 2)
	{
		setError("syntax: recv <filename>");
		return;
	}

	fp = vfs_open(argv[1], O_WRONLY);
	if (!fp)
		return;

	xmodem_recv(fp);
	vfs_close(fp);
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


