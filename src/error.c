/*
 * PiFace
 * © 2013 David Given
 * This file is redistributable under the terms of the 3-clause BSD license.
 * See the file 'Copying' in the root of the distribution for the full text.
 */

#include "globals.h"

char* error;

void clearError(void)
{
	free(error);
	error = NULL;
}

void setError(const char* msg, ...)
{
	va_list ap;
	int bytes;

	clearError();

	va_start(ap, msg);
	bytes = vsnprintf(NULL, 0, msg, ap);
	va_end(ap);

	error = malloc(bytes+1);
	va_start(ap, msg);
	vsnprintf(error, bytes+1, msg, ap);
	va_end(ap);
}

