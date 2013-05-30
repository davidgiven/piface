/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"
#include <termios.h>

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

static void xmodem_send(struct file* fp, int len)
{
	struct termios oldtermios;
	struct termios newtermios;
	uint8_t block;
	uint32_t offset;
	uint32_t thisblocklen;
	int crc16 = 0;
	uint16_t crc;
	uint8_t* buffer;
	uint8_t c;

	printf("Give your local XMODEM receive command now.\n");

	tcgetattr(0, &oldtermios);
	newtermios = oldtermios;
	newtermios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    newtermios.c_oflag &= ~(OPOST);
    newtermios.c_cflag |= (CS8);
    newtermios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	tcsetattr(0, TCSADRAIN, &newtermios);
	buffer = malloc(1024);

	block = 1;
	if (len > 128)
		thisblocklen = 1024;
	else
		thisblocklen = 128;
	for (;;)
	{
		int i;
        read(0, &c, 1);

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
				len -= thisblocklen;
				if (len > 128)
					thisblocklen = 1024;
				else
					thisblocklen = 128;
				break;

			default: /* ignore everything else */
				continue;
        }

        if (thisblocklen == 128)
            c = 1; /* SOH */
		else
			c = 2; /* STX */
		write(1, &c, 1);
		c = block;
		write(1, &c, 1);
		c = ~block;
		write(1, &c, 1);

		vfs_read(fp, offset, buffer, thisblocklen);
		crc = 0;
		for (i=0; i<thisblocklen; i++)
		{
			write(1, &buffer[i], 1);

			if (crc16)
			{
				int j;

				crc16 ^= c<<8;
				for (j=0; j<8; j++)
				{
					crc16 <<= 1;
					if (crc16 & 0x10000)
						crc16 ^= 0x1021;
				}
			}
		}

		if (crc16)
		{
			c = crc16 >> 8;
			write(1, &c, 1);
		}

		c = crc16;
		write(1, &c, 1);
	}

	c = 4; /* EOT */
	write(1, &c, 1);

	free(buffer);
	tcsetattr(0, TCSADRAIN, &oldtermios);
}

static void sx_cb(int argc, const char* argv[])
{
	struct file* fp;
	uint32_t len;

	if (argc != 2)
	{
		setError("syntax: sx <filename>");
		return;
	}

	fp = vfs_open(argv[1], O_RDONLY);
	if (!fp)
		return;

	vfs_info(fp, NULL, &len);
	if (len & 0x7f)
		setError("only files which are a multiple of 128 bytes can be sent");
	else
		xmodem_send(fp, len);

	vfs_close(fp);
}

static void rx_cb(int argc, const char* argv[])
{
	setError("unimplemented");
}


