/*
 * Copyright (c) 2015, Gianluca Guida
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


#ifndef __uk_pfndb_h
#define __uk_pfndb_h

#include <uk/pgalloc.h>
#include <uk/slab.h>

typedef struct {
	uint8_t type;
	union {
		char ptr[0];
		struct pgzentry pz;
		struct slab sh;
		struct {
			uint64_t ref;
			uint64_t wir;
		} usr;
	};
} __packed ipfn_t;

enum {
	/* Invalid must be zero, i.e. status of blank page. */
	PFNT_INVALID = 0,

	PFNT_HEAP,		/* Kernel heap */
	PFNT_FREE_PZ_LONE,	/* GFP unique pagezone */
	PFNT_FREE_PZ_STRT,	/* GFP pagezone start */
	PFNT_FREE_PZ_TERM,	/* GFP pagezone end */
	PFNT_FIXMEM,		/* Fixed memory slab allocation */
	PFNT_USER,		/* User allocated pages */

	/* Keep these two at the highest value, for overlap checking */
	PFNT_FREE,		/* Free Page */
	PFNT_SYSTEM,		/* Generic System Page */
	PFNT_MMIO,		/* I/O Mapped Page */
	PFNT_NUM
};

extern unsigned pfndb_max;
#define pfndb_max() pfndb_max

void pfndb_clear(unsigned);
void pfndb_add(unsigned, uint8_t);
void pfndb_subst(uint8_t, uint8_t);

int pfndb_valid(unsigned);

void pfndb_settype(unsigned, uint8_t);
uint8_t pfndb_type(unsigned);
void pfndb_printstats(void);
void pfndb_printranges(void);

void *pfndb_getptr(unsigned);
void *pfndb_setup(void *, unsigned);
uint64_t pfndb_incref(unsigned pfn);
uint64_t pfndb_decref(unsigned pfn);
uint64_t pfndb_getref(unsigned pfn);
uint64_t pfndb_clrref(unsigned pfn);
uint64_t pfndb_incwir(unsigned pfn);
uint64_t pfndb_decwir(unsigned pfn);
uint64_t pfndb_getwir(unsigned pfn);
uint64_t pfndb_clrwir(unsigned pfn);

#define pfn_is_valid(_pfn) (pfndb_valid((_pfn)))
#define pfn_is_mmio(_pfn) (pfndb_type((_pfn)) == PFNT_MMIO)

/* Reference count of user pages uses in user mappings.  The kernel
 * should never map user pages if mapped and used by a process which
 * is non-current.  */
#define pfn_is_userpage(_pfn) (pfndb_type((_pfn)) == PFNT_USER)
#define pfn_incref(_pfn) pfndb_incref((_pfn))
#define pfn_decref(_pfn) pfndb_decref((_pfn))
#define pfn_clrref(_pfn) pfndb_clrref((_pfn))
#define pfn_getref(_pfn) pfndb_getref((_pfn))
/* Wiring count of user pages in user mappings. */
#define pfn_incwir(_pfn) pfndb_incwir((_pfn))
#define pfn_decwir(_pfn) pfndb_decwir((_pfn))
#define pfn_clrwir(_pfn) pfndb_clrwir((_pfn))
#define pfn_getwir(_pfn) pfndb_getwir((_pfn))

static inline pfn_t __allocuser(void)
{
	pfn_t pfn;

	pfn = pgalloc(1, PFNT_USER, GFP_HIGH);
	pfn_clrref(pfn);
	pfn_clrwir(pfn);
	return pfn;
}

static inline pfn_t __allocuser32(void)
{
	pfn_t pfn;

	pfn = pgalloc(1, PFNT_USER, GFP_MEM32);
	pfn_clrref(pfn);
	pfn_clrwir(pfn);
	return pfn;
}

#endif
