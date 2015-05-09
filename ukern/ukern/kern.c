#include <uk/types.h>
#include <uk/param.h>
#include <uk/elf.h>
#include <ukern/pfndb.h>
#include <ukern/fixmems.h>
#include <ukern/heap.h>
#include <ukern/vmap.h>
#include <ukern/structs.h>
#include <machine/uk/pmap.h>
#include <machine/uk/cpu.h>
#include <machine/uk/locks.h>
#include <machine/uk/platform.h>
#include <machine/uk/pmap.h>
#include <lib/lib.h>
#include <uk/sys.h>
#include "kern.h"

void __xcptframe_setup(struct xcptframe *f, vaddr_t ip, vaddr_t sp);
void __xcptframe_setxcpt(struct xcptframe *f, unsigned long xcpt);
void __xcptframe_save(struct xcptframe *f, void *frame);
void __xcptframe_usrupdate(struct xcptframe *f, vaddr_t usrframe);
void __xcptframe_enter(struct xcptframe *f);

static struct slab threads;

static lock_t sched_lock = 0;
static TAILQ_HEAD(thread_list, thread) running_threads =
TAILQ_HEAD_INITIALIZER(running_threads);
static TAILQ_HEAD(, thread) stopped_threads =
TAILQ_HEAD_INITIALIZER(stopped_threads);

lock_t softirq_lock = 0;
uint64_t softirqs = 0;
#define SOFTIRQ_RESCHED (1 << 0)

int usrpgfault = 0;

int usercpy(void *dst, void *src, size_t sz)
{
	usrpgfault = 1;
	__insn_barrier();

	if (_setjmp(current_cpu()->usrpgfaultctx)) {
		printf("buh!\n");
		usrpgfault = 0;
		return -1;
	}

	memcpy(dst, src, sz);
	__insn_barrier();
	usrpgfault = 0;
	return 0;
}

static struct thread *thnew(void (*__start) (void))
{
	struct thread *th;

	th = structs_alloc(&threads);
	th->pmap = pmap_alloc();
	th->flags = 0;
	th->stack_4k = alloc4k();
	memset(th->stack_4k, 0, 4096);

	_setupjmp(th->ctx, __start, th->stack_4k + 0xff0);
	return th;
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
		_longjmp(th->ctx, 1);
		panic("WTF.");
	}
}

int thxcpt(unsigned xcpt)
{
	struct thread *th = current_thread();

	if (!
	    ((th->flags & (THFL_XCPTENTRY | THFL_IN_XCPTENTRY)) ==
	     (THFL_XCPTENTRY)))
		return -1;

	if (usercpy
	    (th->xcptframe, current_thread()->frame,
	     sizeof(struct xcptframe)))
		return -1;

	th->flags &= ~THFL_IN_USRENTRY;
	__xcptframe_setxcpt(&th->xcptentry, xcpt);
	th->flags |= THFL_IN_XCPTENTRY;
	__insn_barrier();
	__xcptframe_enter(&th->xcptentry);
	/* Not reached */
	return 0;
}

struct cpu *cpu_setup(int id)
{
	struct cpu *cpu;

	cpu = heap_alloc(sizeof(struct cpu));
	cpu->idle_thread = NULL;
       	TAILQ_INIT(&cpu->resched);
	cpu->softirq = 0;
	return cpu;
}

void cpu_softirq_raise(int id)
{
	printf("Raising softirq %d\n", id);
	current_cpu()->softirq |= id;
}

static void do_resched(void);

void do_cpu_softirq(void)
{
	uint64_t si;

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
		case THST_RUNNING:
			spinlock(&sched_lock);
			TAILQ_INSERT_TAIL(&running_threads, th,
					  sched_list);
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

void schedule(void)
{
	struct thread *th = NULL, *oldth = current_thread();

	if (oldth == current_cpu()->idle_thread) {
		oldth = NULL;
		goto _skip_resched;
	}

	switch (oldth->status) {
	case THST_RUNNING:
		spinlock(&sched_lock);
		TAILQ_INSERT_TAIL(&running_threads, oldth, sched_list);
		spinunlock(&sched_lock);
		break;
	case THST_STOPPED:
		spinlock(&sched_lock);
		TAILQ_INSERT_TAIL(&stopped_threads, oldth, sched_list);
		spinunlock(&sched_lock);
		break;
	case THST_DELETED:
		TAILQ_INSERT_TAIL(&current_cpu()->resched, oldth,
				  sched_list);
		cpu_softirq_raise(SOFTIRQ_RESCHED);
		break;
	}

      _skip_resched:
	/* Schedule per-cpu? Actually simpler */
	spinlock(&sched_lock);
	if (!TAILQ_EMPTY(&running_threads)) {
		th = TAILQ_FIRST(&running_threads);
		TAILQ_REMOVE(&running_threads, th, sched_list);
	}
	spinunlock(&sched_lock);

	if (th == NULL)
		th = current_cpu()->idle_thread;

	thswitch(th);
}

__dead void die(void)
{
	pfn_t pfn;
	vaddr_t va;
	struct thread *th = current_thread();

	/* In the future, remove shared mapping  before */
	/* clear user mappings */
	for (va = USERBASE; va < USEREND; va += PAGE_SIZE) {
		pfn = pmap_enter(th->pmap, va, 0, 0);
		if (pfn != PFN_INVALID)
			__freepage(pfn);
	}

	th->status = THST_DELETED;
	schedule();
}

static void idle(void)
{
	while (1) {
		schedule();
		do_cpu_softirq();
		platform_wait();
	}
}

static void populate(vaddr_t addr, size_t sz, pmap_prot_t prot)
{
	int i;
	pfn_t pfn;

	for (i = 0; i < round_page(sz) >> PAGE_SHIFT; i++) {
		pfn = __allocpage(PFNT_USER);
		pmap_enter(NULL, addr + i * PAGE_SIZE, ptoa(pfn), prot);
	}
	pmap_commit(NULL);
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
			populate(ph->va, ph->fsize, PROT_USER_WRX);
			memcpy((void *) ph->va, (void *) ELFOFF(ph->off),
			       ph->fsize);
		}
		if (ph->msize - ph->fsize > 0) {
			populate(ph->va + ph->fsize, ph->msize - ph->fsize,
				 PROT_USER_WRX);
			memset((void *) (ph->va + ph->fsize), 0,
			       ph->msize - ph->fsize);
		}
	}

	return (vaddr_t) hdr->entry;
}

static void __initstart(void)
{
	vaddr_t entry;
	struct thread *th = current_thread();
	extern void *_init_start;

	entry = elfld(_init_start);
	__xcptframe_setup(&th->usrentry, entry, 0);
	th->flags |= THFL_IN_USRENTRY;
	__insn_barrier();
	__xcptframe_enter(&th->usrentry);
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
	set_current_thread(th);
	current_cpu()->idle_thread = th;

	/* We are idle thread now. Create init */

	th = thnew(__initstart);

	spinlock(&sched_lock);
	TAILQ_INSERT_TAIL(&running_threads, th, sched_list);
	spinunlock(&sched_lock);

	cpu_wakeup_aps();
	idle();
}

void kern_bootap(void)
{
	struct thread *th;

	/* initialise idle thread */
	th = structs_alloc(&threads);
	th->pmap = pmap_current();
	th->stack_4k = NULL;
	set_current_thread(th);
	current_cpu()->idle_thread = th;

	idle();
}
