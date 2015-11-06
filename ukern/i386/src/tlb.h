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


#ifndef __i386_tlb_h
#define __i386_tlb_h

#include <machine/uk/pmap.h>

/* NB: the following value are OR'd when set. Take care on usage. */
#define TLBF_NULL   0
#define TLBF_NORMAL 1
#define TLBF_GLOBAL 2

#define restricts_permissions(_o, _n) 1

static __inline int __tlbflushp(l1e_t ol1e, l1e_t nl1e)
{

	/* Previous not present. Don't flush. */
	if (!l1eflags(ol1e) & PG_P)
		return 0;

	/* Mapping a different page. Flush. */
	if (l1epfn(ol1e) != l1epfn(nl1e))
		goto flush;

	if (restricts_permissions(ol1e, nl1e))
		goto flush;

	return TLBF_NULL;

      flush:
	if ((l1eflags(ol1e) & PG_G) || (l1eflags(nl1e) & PG_G))
		return TLBF_GLOBAL;
	return TLBF_NORMAL;
}

void __flush_tlbs(cpumask_t cpu, unsigned flags);
void __flush_tlbs_on_nmi(void);
void __flush_local_tlbs(void);
void __flush_global_tlbs(void);

#endif
