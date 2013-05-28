/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"

struct mmc_interface
{
	volatile uint32_t cmd;
	volatile uint32_t arg;
	volatile uint32_t timeout;
	volatile uint32_t clkdiv;
	volatile uint32_t rsp0;
	volatile uint32_t rsp1;
	volatile uint32_t rsp2;
	volatile uint32_t rsp3;
	volatile uint32_t status;
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

static struct mmc_interface* altmmc;

void mmc_init(void)
{
	uint32_t i;

	altmmc = pi_phys_to_user((void*) 0x7e202000);

	i = altmmc->cmd = 0x8000 | 0; /* GO_IDLE_STATE */

	for (;;)
	{
		uint32_t j = altmmc->cmd;
		if (j != i)
		{
			printf("CMD changed to %08x\n", j);
			i = j;
		}
	}
}
