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


#include <uk/types.h>
#include <uk/param.h>
#include <uk/elf.h>
#include <uk/pfndb.h>
#include <uk/fixmems.h>
#include <uk/heap.h>
#include <uk/vmap.h>
#include <uk/structs.h>
#include <machine/uk/pmap.h>
#include <machine/uk/cpu.h>
#include <machine/uk/locks.h>
#include <machine/uk/machdep.h>
#include <machine/uk/platform.h>
#include <machine/uk/pmap.h>
#include <lib/lib.h>
#include <uk/sys.h>
#include "kern.h"

static struct slab threads;
static cpumask_t cpu_idlemap = 0;

static lock_t sched_lock = 0;
static TAILQ_HEAD(thread_list, thread) running_threads =
TAILQ_HEAD_INITIALIZER(running_threads);
static TAILQ_HEAD(, thread) stopped_threads =
TAILQ_HEAD_INITIALIZER(stopped_threads);

lock_t softirq_lock = 0;
uint64_t softirqs = 0;
#define SOFTIRQ_RESCHED (1 << 0)

void __bad_thing(int user, const char *format, ...)
{
	va_list ap;

	printf("(bad thing at CPU %d): ", cpu_number());
	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);
	printf("\n");

	if (user)
		die();
	else
		__goodbye();
}

/* Use copy_{to,from}_user(), do not call this directly */
int __usrcpy(uaddr_t uaddr, void *dst, void *src, size_t sz)
{

	assert(__chkuaddr(uaddr, sz));

	current_cpu()->usrpgfault = 1;
	__insn_barrier();
	if (_setjmp(current_cpu()->usrpgfaultctx)) {
		current_cpu()->usrpgfault = 0;
		return -1;
	}

	memcpy(dst, src, sz);

	__insn_barrier();
	current_cpu()->usrpgfault = 0;
	return 0;
}

static struct thread *thnew(void (*__start) (void))
{
	struct thread *th;

	th = structs_alloc(&threads);
	th->pmap = pmap_alloc();
	th->stack_4k = alloc4k();
	memset(th->stack_4k, 0, 4096);
	th->userfl = 0;
	th->softintrs = 0;

	_setupjmp(th->ctx, __start, th->stack_4k + 0xff0);
	return th;
}

void thintr(unsigned vect, vaddr_t info)
{
	struct thread *th = current_thread();
	uint32_t ofl = th->userfl;

	th->userfl &= ~THFL_INTR;	/* Restored on IRET */
	usrframe_signal(th->frame, th->sigip, th->sigsp, ofl, vect, info);
}

void thraise(struct thread *th, unsigned vect)
{

	__sync_or_and_fetch(&th->softintrs, (1L << vect));
	wake(th);
}

static void thfree(struct thread *th)
{
	/* thread must not be active, on any cpu */
	pmap_free(th->pmap);
	free4k(th->stack_4k);
	structs_free(th);
}

static void thswitch(struct thread *th)
{
	if (!_setjmp(current_thread()->ctx)) {
		pmap_switch(th->pmap);
		set_current_thread(th);
		if (thread_is_idle(th))
			__sync_or_and_fetch(&cpu_idlemap,
					    (1L << cpu_number()));
		_longjmp(th->ctx, 1);
		panic("WTF.");
	}
}

struct cpu *cpu_setup(int id)
{
	struct cpu *cpu;

	cpu = heap_alloc(sizeof(struct cpu));
	cpu->idle_thread = NULL;
	TAILQ_INIT(&cpu->resched);
	cpu->softirq = 0;
	cpu->usrpgfault = 0;
	return cpu;
}

void cpu_softirq_raise(int id)
{
	printf("Raising softirq %d\n", id);
	current_cpu()->softirq |= id;
}

static void do_resched(void);

void do_softirq(void)
{
	struct thread *th = current_thread();
	uint64_t si;

	if (th->userfl & THFL_INTR) {
		si = __sync_fetch_and_and(&th->softintrs, 0);
		if (si)
			thintr(INTR_EXT, si);
	}

	while (current_cpu()->softirq) {
		si = current_cpu()->softirq;
		current_cpu()->softirq = 0;

		if (si & SOFTIRQ_RESCHED) {
			do_resched();
			si &= ~SOFTIRQ_RESCHED;
		}
	}
}

static void do_resched(void)
{
	struct thread *th, *tmp;

	printf("resched!\n");

	TAILQ_FOREACH_SAFE(th, &current_cpu()->resched, sched_list, tmp) {
		TAILQ_REMOVE(&current_cpu()->resched, th, sched_list);
		switch (th->status) {
		case THST_RUNNABLE:
			spinlock(&sched_lock);
			TAILQ_INSERT_TAIL(&running_threads, th,
					  sched_list);
			printf("Added runnable!\n");
			printf("Waking first idle cpu! (%llx)\n",
			       cpu_idlemap);
			once_cpumask(cpu_idlemap, cpu_ipi(i, IPI_NOP));
			spinunlock(&sched_lock);
			break;
		case THST_STOPPED:
			spinlock(&sched_lock);
			TAILQ_INSERT_TAIL(&stopped_threads, th,
					  sched_list);
			spinunlock(&sched_lock);
			break;
		case THST_DELETED:
			thfree(th);
			break;
		}
	}
}

void wake(struct thread *th)
{
	spinlock(&sched_lock);
	switch (th->status) {
	case THST_DELETED:
		printf("Waking deleted thread?");
	case THST_RUNNABLE:
		break;
	case THST_RUNNING:
		/* Send nmi to cpu */
		break;
	case THST_STOPPED:
		TAILQ_REMOVE(&stopped_threads, th, sched_list);
		th->status = THST_RUNNABLE;
		TAILQ_INSERT_TAIL(&current_cpu()->resched, th, sched_list);
		cpu_softirq_raise(SOFTIRQ_RESCHED);
		break;
	}
	spinunlock(&sched_lock);
}

void schedule(int newst)
{
	struct thread *th = NULL, *oldth = current_thread();

	/* Nothing to do */
	if (newst == THST_RUNNING)
		return;

	if (oldth == current_cpu()->idle_thread) {
		__sync_and_and_fetch(&cpu_idlemap, ~(1L << cpu_number()));
		oldth = NULL;
		goto _skip_resched;
	}

	spinlock(&sched_lock);

	/* Do not sleep if active signals */
	if (newst == THST_STOPPED && thread_has_interrupts(oldth)) {
		spinunlock(&sched_lock);
		return;
	}

	oldth->status = newst;
	switch (oldth->status) {
	case THST_RUNNABLE:
		TAILQ_INSERT_TAIL(&running_threads, oldth, sched_list);
		break;
	case THST_STOPPED:
		TAILQ_INSERT_TAIL(&stopped_threads, oldth, sched_list);
		break;
	case THST_DELETED:
		TAILQ_INSERT_TAIL(&current_cpu()->resched, oldth,
				  sched_list);
		cpu_softirq_raise(SOFTIRQ_RESCHED);
		break;
	}
	spinunlock(&sched_lock);

      _skip_resched:
	/* Schedule per-cpu? Actually simpler */
	spinlock(&sched_lock);
	if (!TAILQ_EMPTY(&running_threads)) {
		th = TAILQ_FIRST(&running_threads);
		TAILQ_REMOVE(&running_threads, th, sched_list);
		th->status = THST_RUNNING;
		th->cpu = cpu_number();
	} else {
		th = current_cpu()->idle_thread;
	}
	spinunlock(&sched_lock);
	thswitch(th);
}

__dead void die(void)
{
	struct thread *th = current_thread();

	/* In the future, remove shared mapping mechanisms before
	 * mappings */
	vmclear(USERBASE, USEREND - USERBASE);
	schedule(THST_DELETED);
}

static struct thread *initth;

static void idle(void)
{
	while (1) {
		do_softirq();
		schedule(THST_STOPPED);
		platform_wait();
	}
}

unsigned vmpopulate(vaddr_t addr, size_t sz, pmap_prot_t prot)
{
	int i, rc, ret = 0;
	pfn_t pfn;

	for (i = 0; i < round_page(sz) >> PAGE_SHIFT; i++) {
		pfn = __allocuser();
		rc = pmap_uenter(NULL, addr + i * PAGE_SIZE, pfn, prot, &pfn);
		assert(!rc && "vmpopulate pmap_uenter");
		if (pfn != PFN_INVALID) {
			__freepage(pfn);
			ret++;
		}
	}
	pmap_commit(NULL);
	return ret;
}

unsigned vmclear(vaddr_t addr, size_t sz)
{
	unsigned ret = 0;
	pfn_t pfn;
	int i, rc;

	for (i = 0; i < round_page(sz) >> PAGE_SHIFT; i++) {
		rc = pmap_uclear(NULL, addr + i * PAGE_SIZE, &pfn);
		assert(!rc && "vmclear pmap_uclear");
		if (pfn != PFN_INVALID) {
			__freepage(pfn);
			ret++;
		}
	}
	pmap_commit(NULL);
	return ret;
}

int vmmap(vaddr_t addr, pmap_prot_t prot)
{
	int ret;
	pfn_t pfn;

	pfn = __allocuser();
	ret = pmap_uenter(NULL, addr, pfn, prot, &pfn);
	pmap_commit(NULL);

	if (pfn != PFN_INVALID) {
		__freepage(pfn);
		ret = 1;
	}
	return ret;
}

int vmunmap(vaddr_t addr)
{
	int ret;
	pfn_t pfn;

	ret = pmap_uenter(NULL, addr, PFN_INVALID, 0, &pfn);
	pmap_commit(NULL);

	if (pfn != PFN_INVALID) {
		__freepage(pfn);
		ret = 1;
	}
	return ret;
}

int vmchprot(vaddr_t addr, pmap_prot_t prot)
{
	int ret;

	ret = pmap_uchprot(NULL, addr, prot);
	pmap_commit(NULL);
	return ret;
}

static vaddr_t elfld(void *elfimg)
{
	int i;
#define ELFOFF(_o) ((void *)((uintptr_t)elfimg + (_o)))

	char elfid[] = { 0x7f, 'E', 'L', 'F', };
	struct elfhdr *hdr = (struct elfhdr *) elfimg;
	struct elfph *ph = (struct elfph *) ELFOFF(hdr->phoff);

	assert(!memcmp(hdr->id, elfid, 4));
	for (i = 0; i < hdr->phs; i++, ph++) {
		if (ph->type != PHT_LOAD)
			continue;
		if (ph->fsize) {
			vmpopulate(ph->va, ph->fsize, PROT_USER_WRX);
			memcpy((void *) ph->va, (void *) ELFOFF(ph->off),
			       ph->fsize);
		}
		if (ph->msize - ph->fsize > 0) {
			vmpopulate(ph->va + ph->fsize,
				   ph->msize - ph->fsize, PROT_USER_WRX);
			memset((void *) (ph->va + ph->fsize), 0,
			       ph->msize - ph->fsize);
		}
	}

	return (vaddr_t) hdr->entry;
}

static void __initstart(void)
{
	vaddr_t entry;
	struct usrframe usrframe;
	extern void *_init_start;

	entry = elfld(_init_start);
	usrframe_setup(&usrframe, entry, 0);
	__insn_barrier();
	usrframe_enter(&usrframe);
	/* Not reached */
}

void kern_boot(void)
{
	struct thread *th;

	printf("Kernel loaded at va %08lx:%08lx\n", UKERNTEXTOFF,
	       UKERNEND);
	/* initialise threads */
	setup_structcache(&threads, thread);

	th = structs_alloc(&threads);
	th->pmap = pmap_current();
	th->stack_4k = NULL;
	th->userfl = 0;
	th->softintrs = 0;
	set_current_thread(th);
	current_cpu()->idle_thread = th;
	/* We are idle thread now. */

	cpu_wakeup_aps();


	/* Create init */
	th = thnew(__initstart);
	spinlock(&sched_lock);
	TAILQ_INSERT_TAIL(&running_threads, th, sched_list);
	spinunlock(&sched_lock);

	idle();
}

void kern_bootap(void)
{
	struct thread *th;

	/* initialise idle thread */
	th = structs_alloc(&threads);
	th->pmap = pmap_current();
	th->stack_4k = NULL;
	th->userfl = 0;
	th->softintrs = 0;
	set_current_thread(th);
	current_cpu()->idle_thread = th;

	idle();
}
