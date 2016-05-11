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
#include <uk/locks.h>
#include <uk/pfndb.h>
#include <uk/fixmems.h>
#include <uk/heap.h>
#include <uk/vmap.h>
#include <uk/structs.h>
#include <machine/uk/pmap.h>
#include <machine/uk/cpu.h>
#include <machine/uk/machdep.h>
#include <machine/uk/platform.h>
#include <machine/uk/pmap.h>
#include <uk/bus.h>
#include <uk/usrdev.h>
#include <lib/lib.h>
#include <uk/sys.h>
#include "kern.h"

static struct slab threads;
static cpumask_t cpu_idlemap = 0;

static struct thread *__kern_init = NULL;

static lock_t sched_lock = 0;
static TAILQ_HEAD(thread_list, thread) running_threads =
TAILQ_HEAD_INITIALIZER(running_threads);
static TAILQ_HEAD(, thread) stopped_threads =
TAILQ_HEAD_INITIALIZER(stopped_threads);

lock_t softirq_lock = 0;
uint64_t softirqs = 0;
#define SOFTIRQ_RESCHED (1 << 0)

static lock_t irqsigs_lock;
LIST_HEAD(, irqsig) irqsigs[MAXIRQS];

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
		current_cpu()->usrpgaddr = 0;
		current_cpu()->usrpgfault = 0;
		return -EFAULT;
	}

	memcpy(dst, src, sz);

	__insn_barrier();
	current_cpu()->usrpgfault = 0;
	current_cpu()->usrpgaddr = 0;
	return 0;
}

int __getperm(struct thread *th, uid_t uid, gid_t gid, mode_t mode)
{
	int perm;

	if (uid == th->euid) {
		perm = (mode & 0700);
		perm >>= 6;
	} else if (gid == th->egid) {
		perm = (mode & 0070);
		perm >>= 3;
	} else {
		perm = (mode & 0007);
	}
	return perm;
}

static pid_t getnewpid(void)
{
	static pid_t ret, pid = 0;

	ret = pid++;

	/* XXX: Okay, this is not serious. This is a hack. A bad one. */
	assert(pid != 0);
	return ret;
}

static void releasepid(pid_t pid)
{
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

	th->pid = getnewpid();

	th->setuid = 0;
	th->ruid = 0;
	th->euid = 0;
	th->suid = 0;
	th->rgid = 0;
	th->egid = 0;
	th->sgid = 0;

	th->usrdev = 0;
	memset(&th->bus, 0, sizeof(th->bus));

	_setupjmp(th->ctx, __start, th->stack_4k + 0xff0);
	return th;
}

static void __childstart(void)
{
	struct thread *th = current_thread();

	/* Child returns zero */
	usrframe_setret(th->frame, 0);
	__insn_barrier();
	usrframe_enter(th->frame);
	/* Not reached */
}

struct thread *thfork(void)
{
	vaddr_t va;
	struct thread *nth, *cth = current_thread();

	nth = structs_alloc(&threads);
	nth->pmap = pmap_copy();
	nth->stack_4k = alloc4k();
	nth->frame = (uint8_t *) nth->stack_4k;
	memcpy(nth->frame, cth->frame, sizeof(struct usrframe));

	nth->userfl = cth->userfl;
	nth->softintrs = cth->softintrs;

	nth->pid = getnewpid();

	nth->children_lock = 0;
	LIST_INIT(&nth->active_children);
	LIST_INIT(&nth->zombie_children);

	nth->setuid = cth->setuid;
	nth->ruid = cth->ruid;
	nth->euid = cth->euid;
	nth->suid = cth->suid;
	nth->rgid = cth->rgid;
	nth->egid = cth->egid;
	nth->sgid = cth->sgid;

	nth->sigip = cth->sigip;
	nth->sigsp = cth->sigsp;

	/* XXX: What to do on fork */
	nth->usrdev = 0;
	memset(&nth->bus, 0, sizeof(nth->bus));
	/* XXX: FORK BUS! */

	/* NB: Stack is mostly at the end of the 4k page.
	 * Frame is at the very beginning. */
	_setupjmp(nth->ctx, __childstart, nth->stack_4k + 0xff0);

	/* Copy the no cow area */
	for (va = ZCOWBASE; va < ZCOWEND; va += PAGE_SIZE) {
		pfn_t pfn;
		int ret;

		/* XXX: Allocate in KERN pool to directly map and
		 * copy, avoiding global vmapping. Fix properly by
		 * implementing local high mem mapping, using only one
		 * slot and a couple of INVLPG's */
		pfn = pgalloc(1, PFNT_USER, GFP_KERN);
		pfndb_clrref(pfn);
		if (copy_from_user((void *) ptova(pfn), va, PAGE_SIZE)) {
			/* Not present */
			pgfree(pfn, 1);
			continue;
		}
		ret = pmap_uenter(nth->pmap, va, pfn, PROT_USER_WR, &pfn);
		assert(!ret && (pfn == PFN_INVALID));
	}
	/* Finished handling the pmaps. */
	pmap_commit(NULL);

	nth->parent = cth;
	spinlock(&cth->children_lock);
	LIST_INSERT_HEAD(&cth->active_children, nth, child_list);
	spinunlock(&cth->children_lock);

	/* No need to flash nth->pmap, not used yet */
	nth->status = THST_RUNNABLE;
	TAILQ_INSERT_TAIL(&current_cpu()->resched, nth, sched_list);
	cpu_softirq_raise(SOFTIRQ_RESCHED);

	return nth;
}

void thintr(unsigned vect, vaddr_t va, unsigned long err)
{
	struct thread *th = current_thread();
	uint32_t ofl = th->userfl;

	th->userfl &= ~THFL_INTR;	/* Restored on IRET */
	usrframe_signal(th->frame, th->sigip, th->sigsp, ofl, vect, va,
			err);
}

void thextintr(unsigned vect, uint64_t sigs)
{
	struct thread *th = current_thread();
	uint32_t ofl = th->userfl;

	th->userfl &= ~THFL_INTR;	/* Restored on IRET */
	usrframe_extint(th->frame, th->sigip, th->sigsp, ofl, vect, sigs);
}

void thraise(struct thread *th, unsigned vect)
{

	assert(vect < MAXSIGNALS);
	__sync_or_and_fetch(&th->softintrs, (1LL << vect));
	wake(th);
}

static void thfree(struct thread *th)
{
	/* thread must not be active, on any cpu */
	pmap_free(th->pmap);
	free4k(th->stack_4k);
	releasepid(th->pid);
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
		usrframe_switch();
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
			thextintr(INTR_EXT, si);
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

	TAILQ_FOREACH_SAFE(th, &current_cpu()->resched, sched_list, tmp) {
		TAILQ_REMOVE(&current_cpu()->resched, th, sched_list);
		switch (th->status) {
		case THST_RUNNABLE:
			spinlock(&sched_lock);
			TAILQ_INSERT_TAIL(&running_threads, th,
					  sched_list);
			once_cpumask(cpu_idlemap, cpu_ipi(i, VECT_NOP));
			spinunlock(&sched_lock);
			break;
		case THST_STOPPED:
			spinlock(&sched_lock);
			TAILQ_INSERT_TAIL(&stopped_threads, th,
					  sched_list);
			spinunlock(&sched_lock);
			break;
		case THST_ZOMBIE:
			spinlock(&th->parent->children_lock);
			LIST_REMOVE(th, child_list);
			LIST_INSERT_HEAD(&th->parent->zombie_children, th,
					 child_list);
			spinunlock(&th->parent->children_lock);
			thraise(th->parent, INTR_CHILD);
			break;
		}
	}
}

void wake(struct thread *th)
{
	spinlock(&sched_lock);
	switch (th->status) {
	case THST_ZOMBIE:
		printf("Waking zombie thread?");
	case THST_RUNNABLE:
		break;
	case THST_RUNNING:
		/* Send nmi to cpu */
		break;
	case THST_STOPPED:
		TAILQ_REMOVE(&stopped_threads, th, sched_list);
		th->status = THST_RUNNABLE;
		TAILQ_INSERT_TAIL(&running_threads, th, sched_list);
		once_cpumask(cpu_idlemap, cpu_ipi(i, VECT_NOP));
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

	/* Do not sleep if active signals */
	if ((newst == THST_STOPPED) && thread_has_interrupts(oldth))
		return;

	spinlock(&sched_lock);
	oldth->status = newst;
	switch (oldth->status) {
	case THST_RUNNABLE:
		TAILQ_INSERT_TAIL(&running_threads, oldth, sched_list);
		break;
	case THST_STOPPED:
		TAILQ_INSERT_TAIL(&stopped_threads, oldth, sched_list);
		break;
	case THST_ZOMBIE:
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

int childstat(struct sys_childstat *cs)
{
	pid_t pid;
	struct thread *child, *th = current_thread();

	spinlock(&th->children_lock);
	child = LIST_FIRST(&th->zombie_children);
	if (child != NULL)
		LIST_REMOVE(child, child_list);
	spinunlock(&th->children_lock);

	if (child == NULL)
		return -ESRCH;

	cs->exit_status = child->exit_status;
	pid = child->pid;
	thfree(child);
	return pid;
}

__dead void __exit(int status)
{
	struct thread *th = current_thread();
	struct thread *child, *tmp;

	if (th->parent == NULL) {
		panic("Killed System Process %d\nAye!\n", th->pid);
	}

	/* In the future, remove shared mapping mechanisms before
	 * mappings */
	bus_remove(&th->bus);
	devremove();
	vmclear(USERBASE, USEREND - USERBASE);
	/* Let INIT inherit children of dead process */
	spinlock(&th->children_lock);
	LIST_FOREACH_SAFE(child, &th->active_children, child_list, tmp) {
		LIST_REMOVE(child, child_list);
		/* Safe. We shouldn't ever be called with th == __kern_init */
		spinlock(&__kern_init->children_lock);
		LIST_INSERT_HEAD(&__kern_init->active_children,
				 child, child_list);
		spinunlock(&__kern_init->children_lock);
		child->parent = __kern_init;
	}
	LIST_FOREACH_SAFE(child, &th->zombie_children, child_list, tmp) {
		LIST_REMOVE(child, child_list);
		/* Safe. We shouldn't ever be called with th == __kern_init */
		spinlock(&__kern_init->children_lock);
		LIST_INSERT_HEAD(&__kern_init->zombie_children,
				 child, child_list);
		spinunlock(&__kern_init->children_lock);
		child->parent = __kern_init;
	}
	spinunlock(&th->children_lock);
	thraise(__kern_init, INTR_CHILD);

	th->exit_status = status;
	schedule(THST_ZOMBIE);
}

__dead void die(void)
{
	__exit(-2);
}

static void idle(void)
{
	while (1) {
		do_softirq();
		schedule(THST_STOPPED);
		platform_wait();
	}
}

int iomap(vaddr_t vaddr, pfn_t mmiopfn, pmap_prot_t prot)
{
	int ret = 0;
	pfn_t opfn;

	if (!pfn_is_mmio(mmiopfn))
		return -EINVAL;

	ret = pmap_uiomap(NULL, vaddr, mmiopfn, prot, &opfn);
	pmap_commit(NULL);

	if (ret < 0)
		return ret;

	if (opfn != PFN_INVALID) {
		__freepage(opfn);
		ret = 1;
	}
	return ret;
}

unsigned vmpopulate(vaddr_t addr, size_t sz, pmap_prot_t prot)
{
	int i, ret = 0;
	pfn_t pfn;

	for (i = 0; i < round_page(sz) >> PAGE_SHIFT; i++) {
		pfn = __allocuser();
		pmap_uenter(NULL, trunc_page(addr) + i * PAGE_SIZE, pfn, prot, &pfn);
		if (pfn != PFN_INVALID) {
			__freepage(pfn);
			ret++;
		}
	}
	pmap_commit(NULL);
	/* Warning: accessing user space without checking (we are
	   single threaded, area just mapped and still locked, it is
	   fine, but future-fragile). */
	memset(trunc_page(addr), 0, round_page(sz));
	return ret;
}

unsigned vmclear(vaddr_t addr, size_t sz)
{
	unsigned ret = 0;
	pfn_t pfn;
	int i;

	for (i = 0; i < round_page(sz) >> PAGE_SHIFT; i++) {
		pmap_uclear(NULL, addr + i * PAGE_SIZE, &pfn);
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
	int ret = 0;
	pfn_t pfn, opfn;

	pfn = __allocuser();
	ret = pmap_uenter(NULL, addr, pfn, prot, &opfn);
	pmap_commit(NULL);

	if (ret < 0) {
		__freepage(pfn);
		return ret;
	}

	if (opfn != PFN_INVALID) {
		__freepage(opfn);
		ret = 1;
	}
	return ret;
}

int vmunmap(vaddr_t addr)
{
	int ret = 0;
	pfn_t pfn;

	ret = pmap_uenter(NULL, addr, PFN_INVALID, 0, &pfn);
	pmap_commit(NULL);

	if (ret < 0)
		return ret;

	if (pfn != PFN_INVALID) {
		__freepage(pfn);
		ret = 1;
	}
	return ret;
}

int vmmove(vaddr_t dst, vaddr_t src)
{
	int ret;
	pfn_t pfn;

	ret = pmap_uchaddr(NULL, src, dst, &pfn);
	pmap_commit(NULL);

	if (ret < 0)
		return ret;

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

int hwcreat(struct sys_hwcreat_cfg *cfg, mode_t mode)
{
	struct thread *th = current_thread();

	if (hwdev_creat(cfg, mode) == NULL)
		return -EEXIST;
	return 0;
}

int devcreat(struct sys_creat_cfg *cfg, unsigned sig, mode_t mode)
{
	struct thread *th = current_thread();

	if (sig > MAXSIGNALS)
		return -EINVAL;

	if (th->usrdev)
		return -EBUSY;

	th->usrdev = usrdev_creat(cfg, sig, mode);
	dprintf("cfg: %llx, %lx, %lx\n", cfg->nameid, cfg->deviceid,
	       cfg->vendorid);
	if (th->usrdev == NULL)
		return -EEXIST;
	return 0;
}

int devpoll(struct sys_poll_ior *polld)
{
	int ret = -ENOENT;
	struct thread *th = current_thread();

	if (th->usrdev)
		ret = usrdev_poll(th->usrdev, polld);
	return ret;
}

int deveio(unsigned id)
{
	int ret = -ENOENT;
	struct thread *th = current_thread();

	if (th->usrdev)
		ret = usrdev_eio(th->usrdev, id);
	return ret;
}

int devirq(unsigned id, unsigned irq)
{
	int ret = -ENOENT;
	struct thread *th = current_thread();

	if (th->usrdev)
		ret = usrdev_irq(th->usrdev, id, irq);
	return ret;
}

int devimport(unsigned id, unsigned iopfn, unsigned va)
{
	int ret = -ENOENT;
	struct thread *th = current_thread();

	if (th->usrdev)
		ret = usrdev_import(th->usrdev, id, iopfn, va);
	return ret;
}

void devremove(void)
{
	struct usrdev *ud;
	struct thread *th = current_thread();

	if (th->usrdev) {
		ud = th->usrdev;
		usrdev_destroy(ud);
		th->usrdev = NULL;
	}
}

int devopen(uint64_t id)
{
	int dd;
	struct thread *th = current_thread();

	dd = bus_plug(&th->bus, id);
	return dd;
}

int devexport(unsigned dd, vaddr_t va, unsigned iopfn)
{
	struct thread *th = current_thread();

	return bus_export(&th->bus, dd, va, iopfn);
}

int devrdcfg(unsigned dd, struct sys_creat_cfg *cfg)
{
	struct thread *th = current_thread();

	return bus_rdcfg(&th->bus, dd, cfg);
}

int devirqmap(unsigned dd, unsigned irq, unsigned sig)
{
	struct thread *th = current_thread();

	return bus_irqmap(&th->bus, dd, irq, sig);
}

int devin(unsigned dd, uint64_t port, uint64_t * valptr)
{
	struct thread *th = current_thread();

	return bus_in(&th->bus, dd, port, valptr);
}

int devout(unsigned dd, uint64_t port, uint64_t val)
{
	struct thread *th = current_thread();

	return bus_out(&th->bus, dd, port, val);
}

int deviomap(unsigned dd, vaddr_t va, paddr_t mmioaddr, pmap_prot_t prot)
{
	struct thread *th = current_thread();

	dprintf("(m %d) ", prot);
	return bus_iomap(&th->bus, dd, va, mmioaddr, prot);
}

int deviounmap(unsigned dd, vaddr_t va)
{
	struct thread *th = current_thread();

	return bus_iounmap(&th->bus, dd, va);
}

void devclose(unsigned dd)
{
	struct thread *th = current_thread();

	bus_unplug(&th->bus, dd);
}

void irqsignal(unsigned irq)
{
	struct irqsig *irqsig;

	assert(irq < MAXIRQS);
	spinlock(&irqsigs_lock);
	LIST_FOREACH(irqsig, &irqsigs[irq], list) {
		if (irqsig->filter && !platform_irqfilter(irqsig->filter))
			continue;
		dprintf("Raising %p:%d\n", irqsig->th, irqsig->sig);
		thraise(irqsig->th, irqsig->sig);
	}
	spinunlock(&irqsigs_lock);
}

void irqunregister(struct irqsig *irqsig)
{

	spinlock(&irqsigs_lock);
	LIST_REMOVE(irqsig, list);
	spinunlock(&irqsigs_lock);
}

int irqregister(struct irqsig *irqsig, unsigned irq, struct thread *th,
		unsigned sig, uint32_t filter)
{
	if (irq >= MAXIRQS)
		return -EINVAL;

	irqsig->th = th;
	irqsig->sig = sig;
	irqsig->filter = filter;

	spinlock(&irqsigs_lock);
	LIST_INSERT_HEAD(&irqsigs[irq], irqsig, list);
	spinunlock(&irqsigs_lock);
	platform_irqon(irq);
	return 0;
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
			/*
			 * memcpy() to user and populate on fault.
			 */

			current_cpu()->usrpgfault = 1;
			__insn_barrier();
			if (_setjmp(current_cpu()->usrpgfaultctx)) {
				dprintf("elf: pop %lx\n",
					current_cpu()->usrpgaddr);
				vmpopulate(current_cpu()->usrpgaddr,
					   PAGE_SIZE, PROT_USER_WRX);
				current_cpu()->usrpgaddr = 0;
			}
			memcpy((void *) ph->va, (void *) ELFOFF(ph->off),
			       ph->fsize);
			__insn_barrier();
			current_cpu()->usrpgfault = 0;
			current_cpu()->usrpgaddr = 0;

		}
		if (ph->msize - ph->fsize > 0) {
			/*
			 * memset() to user and populate on fault.
			 */

			current_cpu()->usrpgfault = 1;
			__insn_barrier();
			if (_setjmp(current_cpu()->usrpgfaultctx)) {
				dprintf("elf: pop %lx\n",
					current_cpu()->usrpgaddr);
				vmpopulate(current_cpu()->usrpgaddr,
					   PAGE_SIZE, PROT_USER_WRX);
				current_cpu()->usrpgaddr = 0;
			}
			memset((void *) (ph->va + ph->fsize), 0,
			       ph->msize - ph->fsize);
			__insn_barrier();
			current_cpu()->usrpgfault = 0;
			current_cpu()->usrpgaddr = 0;
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

	memset(irqsigs, 0, sizeof(irqsigs));
	devices_init();
	printf("Kernel loaded at va %08lx:%08lx\n", UKERNTEXTOFF,
	       UKERNEND);
	/* initialise threads */
	setup_structcache(&threads, thread);

	th = structs_alloc(&threads);
	th->pmap = pmap_current();
	th->stack_4k = NULL;
	th->userfl = 0;
	th->softintrs = 0;

	th->pid = getnewpid();
	th->parent = NULL;
	th->children_lock = 0;
	LIST_INIT(&th->active_children);
	LIST_INIT(&th->zombie_children);

	th->setuid = 0;
	th->ruid = 0;
	th->euid = 0;
	th->suid = 0;
	th->rgid = 0;
	th->egid = 0;
	th->sgid = 0;

	set_current_thread(th);
	current_cpu()->idle_thread = th;
	/* We are idle thread now. */

	cpu_wakeup_aps();

	/* Create init */
	th = thnew(__initstart);
	th->parent = NULL;
	th->children_lock = 0;
	LIST_INIT(&th->active_children);
	LIST_INIT(&th->zombie_children);
	spinlock(&sched_lock);
	TAILQ_INSERT_TAIL(&running_threads, th, sched_list);
	spinunlock(&sched_lock);
	__kern_init = th;
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

	th->pid = getnewpid();
	th->parent = NULL;
	th->children_lock = 0;
	LIST_INIT(&th->active_children);
	LIST_INIT(&th->zombie_children);

	th->setuid = 0;
	th->ruid = 0;
	th->euid = 0;
	th->suid = 0;
	th->rgid = 0;
	th->egid = 0;
	th->sgid = 0;

	set_current_thread(th);
	current_cpu()->idle_thread = th;

	idle();
}
