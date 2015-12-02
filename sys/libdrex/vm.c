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


#include <sys/types.h>
#include <machine/vmparam.h>
#include <microkernel.h>
#include <drex/drex.h>

extern void _scode __asm("_scode");
extern void _ecode __asm("_ecode");
extern void _sdata __asm("_sdata");
extern void _end __asm("_end");

#define VACOW (1L*1024*1024*1024)

static const void *maxbrk = (void *) (1LL * 1024 * 1024 * 1024);
static void *brkaddr = (void *) -1;

static inline void *__getbrk(void)
{
	if (__predict_false(brkaddr == (void *) -1))
		brkaddr = &_end;
	return brkaddr;
}

static vm_prot_t _resolve_va(vaddr_t va)
{
	unsigned pfn = va >> PAGE_SHIFT;

	if (!pfn)
		return VM_PROT_NIL;

	if (va < (vaddr_t) __getbrk()) {
		/* Before the BRK */

		if (va > (vaddr_t) & _sdata)
			return VM_PROT_RW;
		if (va < (vaddr_t) & _ecode && va > (vaddr_t) & _scode)
			return VM_PROT_RX;
		/* Below the start of code lies the NO-COW zone. We
		 * do not demand-page it, and it is unaffected by
		 * fork(). Any fault is an actual fault. */
		return VM_PROT_NIL;
	}

	/* Search for mmap areas */
	return VM_PROT_NIL;
}

int drex_brk(void *nbrk)
{
	if (nbrk < &_end || nbrk >= maxbrk)
		return -1;
	brkaddr = nbrk;
	return 0;
}

void *drex_sbrk(int inc)
{
	void *old = __getbrk();
	void *new = old + inc;

	old = brkaddr;
	if (drex_brk(new))
		return (void *) -1;

	return old;
}

int __sys_pgfaulthandler(vaddr_t va, u_long err, struct intframe *f)
{
	int i, r;
	vm_prot_t prot = _resolve_va(va);
	unsigned reason = err & PG_ERR_REASON_MASK;

	printf("Exception handler, pagefault at addr %lx (%lx)!\n", va,
	       err);
	printf("Should be %d\n", prot);

	if (prot == VM_PROT_NIL) {
		printf("Segmentation fault, should be handled\n");
		sys_die();
	}

	switch (reason) {
	case PG_ERR_REASON_NOTP:
		/* XXX: Find if we need to swap in from somewhere */
		/* XXX: For now, just populate new page */
		printf("_: mapping %lx with prot %x\n", va, prot);
		vmmap(va, prot);
		return 0;
	case PG_ERR_REASON_PROT:
		if ((err & (PG_ERR_INFO_COW | PG_ERR_INFO_WRITE))
		    == (PG_ERR_INFO_COW | PG_ERR_INFO_WRITE)) {
			vaddr_t pg = va & ~PAGE_MASK;

			printf("_: cow fault %lx\n", va);
			vmmap(VACOW, VM_PROT_RW);
			memcpy((void *) VACOW, (void *) pg, PAGE_SIZE);
			sys_move(va, VACOW);
			return 0;
		} else if ((err & PG_ERR_INFO_WRITE)
			   && ((prot == VM_PROT_RW)
			       || (prot == VM_PROT_WX))) {
			printf("_x: fixing wfault %lx with prot %d\n", va,
			       prot);
			printf("%d: vmchprot()\n", vmchprot(va, prot));
			return 0;
		}
		break;
	}
	sys_die();
}
