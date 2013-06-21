/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"
#include "diskio.h"

#if defined TARGET_PI

/* See the SD card spec at:
 *
 *  https://www.sdcard.org/downloads/pls/simplified_specs/part1_410.pdf
 *
 *  http://www.chlazza.net/sdcardinfo.html
 *  http://wiki.seabright.co.nz/wiki/SdCardProtocol.html
 */

struct mmc_interface
{
	volatile uint32_t cmd;     /* 00 */
	volatile uint32_t arg;     /* 04 */
	volatile uint32_t timeout; /* 08 */
	volatile uint32_t clkdiv;  /* 0c */
	volatile uint32_t rsp0;    /* 10 */
	volatile uint32_t rsp1;    /* 14 */
	volatile uint32_t rsp2;    /* 18 */
	volatile uint32_t rsp3;    /* 1c */
	volatile uint32_t status;  /* 20 */
	volatile uint32_t unk_0x24;
	volatile uint32_t unk_0x28;
	volatile uint32_t unk_0x2c;
	volatile uint32_t vdd;
	volatile uint32_t edm;
	volatile uint32_t host_cfg;
	volatile uint32_t hbct;
	volatile uint32_t data;
	volatile uint32_t unk_0x44;
	volatile uint32_t unk_0x48;
	volatile uint32_t unk_0x4c;
	volatile uint32_t hblc;
};

struct gpio_interface
{
	volatile uint32_t fsel0;
	volatile uint32_t fsel1;
	volatile uint32_t fsel2;
	volatile uint32_t fsel3;
	volatile uint32_t fsel4;
	volatile uint32_t fsel5;
	volatile uint32_t set0;
	volatile uint32_t set1;
	volatile uint32_t clr0;
	volatile uint32_t clr1;
	volatile uint32_t lev0;
	volatile uint32_t lev1;
	volatile uint32_t _padding[16];
	volatile uint32_t pud;
	volatile uint32_t pudclk0;
	volatile uint32_t pudclk1;
};

enum
{
	/* cmd register */

	MMC_ENABLE = 1<<15,
    MMC_FAIL = 1<<14,
    MMC_BUSY = 1<<11,
    MMC_NO_RSP = 1<<10,
    MMC_LONG_RSP = 1<<9,
    MMC_WRITE = 1<<7,
    MMC_READ = 1<<6,

    /* status register */

    MMC_FIFO_STATUS = 1<<0
};

static struct mmc_interface* altmmc;
static struct gpio_interface* gpio;

static int sdhc;
static int highcap;
static uint32_t partition_offset;

static void read_block(uint32_t sector, uint32_t* buffer);
static void write_block(uint32_t sector, uint32_t* buffer);

static void wait_for_mmc(void)
{
	while (altmmc->cmd & MMC_ENABLE)
		;
}

static uint32_t mmc_rpc(uint32_t cmd, uint32_t arg)
{
	uint8_t e;

	wait_for_mmc();

	e = altmmc->status;
	if (e)
		altmmc->status &= e;

	altmmc->arg = arg;
	altmmc->cmd = MMC_ENABLE | cmd;

	return altmmc->status;
}

void mmc_init(void)
{
	uint32_t i;

	altmmc = pi_phys_to_user((void*) 0x7e202000);
	gpio = pi_phys_to_user((void*) 0x7e200000);

	gpio->fsel4 = 0x24000000;
	gpio->fsel5 = 0x924;
    gpio->pud = 2;

	altmmc->clkdiv = 0x96;
	altmmc->host_cfg = 0xa;
    altmmc->vdd = 0x1;

	printf("[mounting SD card: ");
	fflush(stdout);

	altmmc->cmd = 0;
	mmc_rpc(0, 0); /* GO_IDLE_STATE */

	sdhc = 0;

	/* Test for SDHC cards. */
	i = mmc_rpc(8, 0x155); /* SEND_IF_COND */
	wait_for_mmc();
	if (!i && ((altmmc->rsp0 & 0xff) == 0x55))
	{
		printf("SDHCv2: ");
		fflush(stdout);
		sdhc = 2;
	}
	else
	{
		printf("SDv1: ");
		sdhc = 1;
	}
	fflush(stdout);

	/* Enable high capacity mode (if available). */

	for (;;)
	{
		mmc_rpc(55, 0); /* APP_CMD */
		i = mmc_rpc(41, (sdhc==2) ? 0x40100000 : 0x00100000); /* SD_SEND_OP_CMD */
		wait_for_mmc();

		if ((i == 0) && (altmmc->rsp0 & (1<<31)))
			break;
		millisleep(100);
	}

	highcap = !!(altmmc->rsp0 & (1<<30));
	if (highcap)
		printf("high capacity: ");
	fflush(stdout);

	/* Get the card's RCA, and select it */

	{
		uint32_t rca;

		mmc_rpc(MMC_LONG_RSP | 2, 0); /* ALL_SEND_CID */
        wait_for_mmc();

        mmc_rpc(3, 0); /* SEND_RELATIVE_RCA */
        wait_for_mmc();
        rca = altmmc->rsp0 & 0xffff0000;

		mmc_rpc(7, rca); /* SELECT_CARD */
		wait_for_mmc();
	}

	/* Select 512 byte blocks. */

    mmc_rpc(16, 512); /* SET_BLOCKLEN */

	altmmc->clkdiv = 0;

	{
		int partition;

		uint8_t* buffer = malloc(512);
		partition_offset = 0;
		read_block(0, (uint32_t*) buffer);

		if ((buffer[510] == 0x55) && (buffer[511] == 0xaa))
		{
			partition = -1;
			for (i=0; i<4; i++)
			{
				uint8_t* p = &buffer[0x1be + i*16];
				switch (p[4])
				{
	                case 0x01: /* FAT12 */
	                case 0x04: /* FAT16, <32MB */
	                case 0x06: /* FAT16, >32MB */
	                case 0x0b: /* FAT32 */
	                case 0x0c: /* FAT32X */
	                case 0x0e: /* FAT16X */
	                    partition = i;
	                    partition_offset = p[8] | (p[9]<<8) | (p[10]<<16) | (p[11]<<24);
				}

				if (partition != -1)
					break;
			}

			printf("partition %d @ 0x%08x]\n", partition, partition_offset);
		}
		else
			printf("whole partition mode]\n");

		fflush(stdout);

		free(buffer);
	}
}

 static void read_block(uint32_t sector, uint32_t* buffer)
 {
 	int i;
 	int count;
 	int crcfailed;

 	sector += partition_offset;
 	#if 0
 		printf("read sector %d\n", sector);
 		fflush(stdout);
 	#endif

 	if (!highcap)
 		sector <<= 9;

 	for (;;)
 	{
 		crcfailed = 0;

 	    i = mmc_rpc(MMC_READ | MMC_BUSY | 18, sector); /* READ_MULTIPLE_BLOCK */
 	    wait_for_mmc();

 	    for (i=0; i<128; i++)
 	    {
 			while (!(altmmc->status & MMC_FIFO_STATUS))
 				;

 			if (altmmc->status != MMC_FIFO_STATUS)
 			{
 				crcfailed = 1;
 				#if 0
 					printf("[block retry]\n");
 					fflush(stdout);
 				#endif
 				break;
 			}

 			buffer[i] = altmmc->data;
 			#if 0
 				if (i > 120)
 					printf("%d %08x %08x\n", i, buffer[i], altmmc->status);
 			#endif
 	    }

 		mmc_rpc(12, 0); /* STOP_TRANSMISSION */

 	    if (!crcfailed)
 	    {
			millisleep(10);
 	        break;
 	    }
 	}
 }

static void write_block(uint32_t sector, uint32_t* buffer)
{
	int i;
	int count;
	int crcfailed;

	sector += partition_offset;
	#if 0
		printf("write sector %d\n", sector);
		fflush(stdout);
	#endif

	if (!highcap)
		sector <<= 9;

	for (;;)
	{
		crcfailed = 0;

	    i = mmc_rpc(MMC_WRITE | MMC_BUSY | 25, sector); /* WRITE_MULTIPLE_BLOCK */
	    wait_for_mmc();

	    for (i=0; i<128; i++)
	    {
			altmmc->data = buffer[i];

			while (!(altmmc->status & MMC_FIFO_STATUS))
				;

			if (altmmc->status != MMC_FIFO_STATUS)
			{
				crcfailed = 1;
				#if 0
					printf("[block retry]\n");
					fflush(stdout);
				#endif
				break;
			}
	    }

		mmc_rpc(12, 0); /* STOP_TRANSMISSION */

	    if (!crcfailed)
	    {
			millisleep(10);
	        break;
	    }
	}
	fflush(stdout);
}

void mmc_deinit(void)
{
}

/* FatFS's interface. */

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber (0..) */
)
{
	return 0;
}

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber (0..) */
)
{
	return 0;
}

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	BYTE count		/* Number of sectors to read (1..128) */
)
{
	while (count--)
	{
		read_block(sector, (uint32_t*) buff);
		sector++;
		buff += 512;
	}
	return 0;
}

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	BYTE count			/* Number of sectors to write (1..128) */
)
{
	while (count--)
	{
		write_block(sector, (uint32_t*) buff);
		sector++;
		buff += 512;
	}
	return 0;
}
#endif

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	switch (cmd)
	{
		case CTRL_SYNC:
			return 0;

		case GET_SECTOR_SIZE:
			*(WORD*)buff = 512;
			return 0;

		case GET_SECTOR_COUNT:
			*(WORD*)buff = 0;
			return 0;

		case GET_BLOCK_SIZE:
			*(WORD*)buff = 1;
			return 0;

        case CTRL_ERASE_SECTOR:
        	return 0;
    }

	return RES_PARERR;
}
#endif

DWORD get_fattime(void)
{
	return 0;
}

#endif
