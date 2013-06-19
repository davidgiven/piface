/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"

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

static void wait_for_mmc(void)
{
	fflush(stdout);
	while (altmmc->cmd & (MMC_ENABLE|MMC_BUSY))
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

	return altmmc->status & 0xe8;
}

void mmc_init(void)
{
	uint32_t i;

	altmmc = pi_phys_to_user((void*) 0x7e202000);
	gpio = pi_phys_to_user((void*) 0x7e200000);

	printf("Set up GPIO pins...\n");
	gpio->fsel4 = 0x24000000;
	gpio->fsel5 = 0x924;
    gpio->pud = 2;

	printf("Init ALTMMC...\n");
	altmmc->clkdiv = 0x96;
	altmmc->host_cfg = 0xa;
    altmmc->vdd = 0x1;

	printf("Reset card...\n");
	altmmc->cmd = 0;
	mmc_rpc(0, 0); /* GO_IDLE_STATE */

	sdhc = 0;

	/* Test for SDHC cards. */
	i = mmc_rpc(8, 0x155); /* SEND_IF_COND */
	wait_for_mmc();
	if (!i && ((altmmc->rsp0 & 0xff) == 0x55))
	{
		printf("Found SDHC v2 card\n");
		sdhc = 2;
	}
	else
	{
		printf("Found SD v1 card\n");
		sdhc = 1;
	}

	/* Enable high capacity mode (if available). */

	for (;;)
	{
		mmc_rpc(55, 0); /* APP_CMD */
		i = mmc_rpc(41, (sdhc==2) ? 0x40100000 : 0); /* SD_SEND_OP_CMD */
		wait_for_mmc();

		if ((i == 0) && (altmmc->rsp0 & (1<<31)))
			break;
		millisleep(100);
	}

	highcap = !!(altmmc->rsp0 & (1<<30));
	if (highcap)
		printf("High capacity mode\n");

	/* Get the card's RCA, and select it */

	{
		uint32_t rca;

		mmc_rpc(MMC_LONG_RSP | 2, 0); /* ALL_SEND_CID */
        wait_for_mmc();
        printf("all_send_cid: %08x %08x %08x %08x\n",
            altmmc->rsp0, altmmc->rsp1,
            altmmc->rsp2, altmmc->rsp3);

        mmc_rpc(3, 0); /* SEND_RELATIVE_RCA */
        wait_for_mmc();
        rca = altmmc->rsp0 & 0xffff0000;
        printf("RCA is 0x%04x\n", rca>>16);

		mmc_rpc(7, rca); /* SELECT_CARD */
		wait_for_mmc();

        printf("%08x %08x %08x %08x\n",
            altmmc->rsp0, altmmc->rsp1,
            altmmc->rsp2, altmmc->rsp3);
	}

	/* Select 512 byte blocks. */

    mmc_rpc(16, 512); /* SET_BLOCKLEN */

}

void mmc_read_block(uint32_t sector, uint32_t* buffer)
{
	int i;

	if (!highcap)
		sector <<= 9;

    mmc_rpc(MMC_READ | 17, sector);
    wait_for_mmc();

    for (i=0; i<128; i++)
    {
	    while (!(altmmc->status & MMC_FIFO_STATUS))
	        ;

		buffer[i] = altmmc->data;
    }
}

#endif
