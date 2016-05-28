/*
 * Copyright (c) 2016, Gianluca Guida
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <sys/types.h>
#include <stdint.h>

/* Little fun. */

static int chenc(char c)
{
	int r;

	if (c >= 'A' && c <= 'Z')
		return c - 'A' + 1;
	if (c >= 'a' && c <= 'z')
		return c - 'a' + 1;
	if (c >= '0' && c <= '9')
		return c - '0' + 036;

	return c == ' ' ? 0 : c == '$' ? 033 : c == '.' ? 034 : 035;
}

static char chdec(int s40)
{
	if (!s40)
		return ' ';
	if (s40 <= 032)
		return 'A' + s40 - 1;
	if (s40 >= 036)
		return '0' + s40 - 036;

	return s40 == 033 ? '$' : s40 == 034 ? '.' : '?';
}

uint64_t squoze(char *string)
{
	char *ptr = string, *max = ptr + 12;
	uint64_t sqz = 0;

	while ( (*ptr != '\0') && (ptr < max) ) {
		sqz *= 40;
		sqz += chenc(*ptr++);
	}

	/* Pad, to get first character at top bits */
	while (ptr++ <  max)
		sqz *= 40;

	return sqz;
}

size_t unsquozelen(uint64_t enc, size_t len, char *string)
{
	uint64_t cut = 1LL*40*40 * 40*40*40 * 40*40*40 * 40*40*40 ;
	char *ptr = string;

	while (enc && len-- > 0) {
	        *ptr++ = chdec((enc / cut) % 40);
		enc = (enc % cut) * 40;
	}
	if (len)
		memset(ptr, 0, len);
	return ptr - string;
}

