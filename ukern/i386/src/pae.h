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


#ifndef __i386_pae_h
#define __i386_pae_h

#include <uk/types.h>
#include <uk/param.h>
#include <machine/uk/ukparam.h>

#define L2SHIFT    L2_SHIFT
#define L2MASK     (0x3 << L2SHIFT)
#define L2OFF(_a)  (((uintptr_t)(_a) & L2MASK) >> L2SHIFT)
#define L2VA(_a)   (((_a) & 0x3) << L2SHIFT)

#define L1SHIFT    L1_SHIFT
#define L1MASK     (0x1ff << L1SHIFT)
#define L1OFF(_a)  (((uintptr_t)(_a) & L1MASK) >> L1SHIFT)
#define L1VA(_a)   (((_a) & 0x1ff) << L1SHIFT)

/* L0: Actual PAE PTE, used only for linear maps here. Sorry. */
#define L0SHIFT    12
#define L0MASK     (0x1ff << L0SHIFT)
#define L0VA(_a)   (((_a) & 0x1ff) << L0SHIFT)


#define KL1_SDMAP  L1OFF(KVA_SDMAP)
#define KL1_EDMAP  L1OFF(KVA_EDMAP)

#define PG_P       1
#define PG_W       2
#define PG_U       4
#define PG_A       0x20
#define PG_D       0x40
#define PG_S       0x80
#define PG_G       0x100
#define PG_AVAIL   0xe00
#if 0
#define PG_NX      0x8000000000000000LL
#else
#define PG_NX      0
#endif

#define PG_AVAIL_NORMAL  (0L << 9)
#define PG_AVAIL_COW     (1L << 9)
#define PG_AVAIL_WIRED   (2L << 9)
#define PG_AVAIL_IOMAP   (4L << 9)

#define __paeoffva(_l2,_l1,_l0) (L2VA(_l2) + L1VA(_l1) + L0VA(_l0))
#define __val1tbl(_va) ((l1e_t *)__paeoffva(2, LINOFF+ 2, LINOFF + L2OFF(_va)))
#define __val2tbl(_va) ((l2e_t *)__paeoffva(2, LINOFF +2, LINOFF + 2) + LINOFF)

#define trunc_4k(_a) ((uintptr_t)(_a) & 0xfffff000)
#define mkl1e(_a, _f) (trunc_page((paddr_t)(_a)) | PG_S | (_f))
#define mkl2e(_a, _f) (trunc_4k((paddr_t)(_a)) | (_f))
/* Linear mapping must use 4k mapping. */
#define mklinl1e(_a, _f) (trunc_4k((paddr_t)(_a)) | (_f))

/* We use 2Mb pages, PAE has only 2 levels for to us */
typedef uint64_t l2e_t;
typedef uint64_t l1e_t;

#define l1epfn(_l1e) ((_l1e)>>PAGE_SHIFT)
#define l1eflags(_l1e) ((_l1e) & 0x8000000000000fffULL)
#define l1eprotfl(_l1e) ((_l1e) & (PG_NX|PG_U|PG_W|PG_P))
#define l1etype(_l1e) ((_l1e) & (PG_AVAIL))

#define l1e_mknormal(_l1e) ((_l1e) & ~PG_AVAIL)
#define l1e_mkcow(_l1e) (((_l1e) & ~(PG_AVAIL|PG_W)) | PG_AVAIL_COW)
#define l1e_mkwired(_l1e) (l1e_mknormal(_l1e) | PG_AVAIL_WIRED)
#define l1e_mkiomap(_l1e) (l1e_mknormal(_l1e) | PG_AVAIL_IOMAP)

#define l1e_present(_l1e) ((_l1e) & PG_P)
#define l1e_normal(_l1e)					\
	(((_l1e) & (PG_AVAIL|PG_P)) == (PG_AVAIL_NORMAL|PG_P))
#define l1e_wired(_l1e)					\
	(((_l1e) & (PG_AVAIL|PG_P)) == (PG_AVAIL_WIRED|PG_P))
#define l1e_cow(_l1e)						\
	(((_l1e) & (PG_AVAIL|PG_P)) == (PG_AVAIL_COW|PG_P))
#define l1e_iomap(_l1e)						\
	(((_l1e) & (PG_AVAIL|PG_P)) == (PG_AVAIL_IOMAP|PG_P))
#define l1e_external(_l1e) (l1e_wired(_l1e) || l1e_iomap(_l1e))

#endif
