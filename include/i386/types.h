/* *INDENT-OFF* */ /* Imported from NetBSD -- MHDIFFIGNORE */
/*	$NetBSD: types.h,v 1.79 2014/04/24 19:23:00 christos Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)types.h	7.5 (Berkeley) 3/9/91
 */

#ifndef	_I386_MACHTYPES_H_
#define	_I386_MACHTYPES_H_

#ifndef _DREX_SOURCE
#ifdef _KERNEL_OPT
#include "opt_xen.h"
#endif
#endif /* _DREX_SOURCE */

#include <sys/cdefs.h>
#include <sys/featuretest.h>
#include <machine/int_types.h>

#if defined(_KERNEL)
typedef struct label_t {
	int val[6];
} label_t;
#endif

#if defined(_NETBSD_SOURCE) || defined(_DREX_SOURCE)
#if defined(_KERNEL)

/*
 * XXX JYM for now, in kernel paddr_t can be 32 or 64 bits, depending
 * on PAE. Revisit when paddr_t becomes 64 bits for !PAE systems.
 */
#ifdef PAE
typedef __uint64_t	paddr_t;
typedef __uint64_t	psize_t;
#define	PRIxPADDR	"llx"
#define	PRIxPSIZE	"llx"
#define	PRIuPSIZE	"llu"
#else /* PAE */
typedef unsigned long	paddr_t;
typedef unsigned long	psize_t;
#define	PRIxPADDR	"lx"
#define	PRIxPSIZE	"lx"
#define	PRIuPSIZE	"lu"
#endif /* PAE */

#else /* _KERNEL */
/* paddr_t is always 64 bits for userland */
typedef __uint64_t	paddr_t;
typedef __uint64_t	psize_t;
#define	PRIxPADDR	"llx"
#define	PRIxPSIZE	"llx"
#define	PRIuPSIZE	"llu"

#endif /* _KERNEL */

typedef unsigned long	vaddr_t;
typedef unsigned long	vsize_t;
#define	PRIxVADDR	"lx"
#define	PRIxVSIZE	"lx"
#define	PRIuVSIZE	"lu"
#endif /* _NETBSD_SOURCE || _DREX_SOURCE */

typedef int		pmc_evid_t;
typedef __uint64_t	pmc_ctr_t;
typedef int		register_t;
#define	PRIxREGISTER	"x"

typedef	volatile unsigned char		__cpu_simple_lock_t;

/* __cpu_simple_lock_t used to be a full word. */
#define	__CPU_SIMPLE_LOCK_PAD

#define	__SIMPLELOCK_LOCKED	1
#define	__SIMPLELOCK_UNLOCKED	0

/* The x86 does not have strict alignment requirements. */
#define	__NO_STRICT_ALIGNMENT

#define	__HAVE_NEW_STYLE_BUS_H
#define	__HAVE_CPU_DATA_FIRST
#define	__HAVE_CPU_COUNTER
#define	__HAVE_CPU_BOOTCONF
#define	__HAVE_MD_CPU_OFFLINE
#define	__HAVE_SYSCALL_INTERN
#define	__HAVE_MINIMAL_EMUL
#define	__HAVE_OLD_DISKLABEL
#if defined(_KERNEL) && !defined(_RUMPKERNEL) && !defined(_RUMP_NATIVE_ABI)
/*
 * Processors < i586 do not have cmpxchg8b, and we compile for i486
 * by default in userland. The kernel tsc driver uses them though,
 * and handles < i586 * by patching. We don't want to expose them in
 * userland, that is why we exclude rump.
 */
#define __HAVE_ATOMIC64_OPS
#endif
#define	__HAVE_ATOMIC_AS_MEMBAR
#define	__HAVE_CPU_LWP_SETPRIVATE
#define	__HAVE_INTR_CONTROL
#define	__HAVE_MM_MD_OPEN
#ifndef _DREX_SOURCE
#define	__HAVE___LWP_GETPRIVATE_FAST
#define	__HAVE_TLS_VARIANT_II
#define	__HAVE_COMMON___TLS_GET_ADDR
#endif

#if defined(_KERNEL)
#define	__HAVE_RAS
#endif

#endif	/* _I386_MACHTYPES_H_ */