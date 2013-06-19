/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"

void millisleep(uint32_t s)
{
	struct timeval t;
	fd_set rds, wrs, exs;

	t.tv_sec = s/1000;
	t.tv_usec = (s%1000)*1000;
	FD_ZERO(&rds);
	FD_ZERO(&wrs);
	FD_ZERO(&exs);

	fflush(stdout);
	select(0, &rds, &wrs, &exs, &t);
}


