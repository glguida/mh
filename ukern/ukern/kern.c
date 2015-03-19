#include <uk/types.h>
#include <uk/param.h>
#include <uk/elf.h>
#include <ukern/pfndb.h>
#include <ukern/fixmems.h>
#include <ukern/heap.h>
#include <ukern/vmap.h>
#include <ukern/thread.h>
#include <ukern/structs.h>
#include <machine/uk/pmap.h>
#include <machine/uk/cpu.h>
#include <machine/uk/locks.h>
#include <machine/uk/platform.h>
#include <machine/uk/pmap.h>
#include <lib/lib.h>
#include "kern.h"

void __usrentry_setup(struct usrentry *ue, vaddr_t ip);
void __usrentry_enter(void *frame);

static struct slab threads;

static lock_t sched_lock = 0;

static TAILQ_HEAD(thread_list, thread) running_threads =
TAILQ_HEAD_INITIALIZER(running_threads);
static TAILQ_HEAD(, thread) stopped_threads =
TAILQ_HEAD_INITIALIZER(stopped_threads);

lock_t softirq_lock = 0;
uint64_t softirqs = 0;
#define SOFTIRQ_RESCHED (1 << 0)

struct thread *thnew(void (*__start) (void))
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

void thfree(struct thread *th)
{
	/* thread must not be active, on any cpu */
	pmap_free(th->pmap);
	free4k(th->stack_4k);
	structs_free(th);
}

void thdel(struct thread *th)
{
	assert(th == NULL || th == current_thread());


}

void thswitch(struct thread *th)
{
	if (!_setjmp(current_thread()->ctx)) {
		pmap_switch(th->pmap);
		set_current_thread(th);
		_longjmp(th->ctx, 1);
		panic("WTF.");
	}
}

int thpgfault(void)
{
	return -1;
}

void cpu_softirq_raise(int id)
{

	printf("Raising softirq %d\n", id);
	current_cpu()->softirq |= id;
}

void do_resched(void);

void do_cpu_softirq(void)
{
	uint64_t si;
	struct thread *th, *tmp;

	while (current_cpu()->softirq) {
		si = current_cpu()->softirq;
		current_cpu()->softirq = 0;

		if (si & SOFTIRQ_RESCHED) {
			do_resched();
			si &= ~SOFTIRQ_RESCHED;
		}
	}
}

void do_resched(void)
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

	if (oldth == current_cpu()->idle_thread)
		oldth = NULL;

	/* Schedule per-cpu? Actually simpler */
	spinlock(&sched_lock);
	if (oldth && thread_is_runnable(oldth) && !thread_is_idle(oldth))
		TAILQ_INSERT_TAIL(&running_threads, oldth, sched_list);
	if (!TAILQ_EMPTY(&running_threads)) {
		th = TAILQ_FIRST(&running_threads);
		TAILQ_REMOVE(&running_threads, th, sched_list);
	}
	spinunlock(&sched_lock);

	if (th == NULL)
		th = current_cpu()->idle_thread;

	thswitch(th);
}

void die(void)
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
	TAILQ_INSERT_TAIL(&current_cpu()->resched, th, sched_list);
	cpu_softirq_raise(SOFTIRQ_RESCHED);
	schedule();
}

void idle(void)
{
	while (1) {
		schedule();
		do_cpu_softirq();
		platform_wait();
	}
}

static void populate(vaddr_t addr, size_t sz, pmap_prot_t prot)
{
	pfn_t pfn;

	pfn = __allocpage(PFNT_USER);
	pmap_enter(NULL, addr, ptoa(pfn), prot);
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
			populate(ph->va, ph->fsize, PROT_USER_WRX);
			memset((void *) (ph->va + ph->fsize), 0,
			       ph->msize - ph->fsize);
		}
	}

	return (vaddr_t) hdr->entry;
}

void __initstart(void)
{
	vaddr_t entry;
	struct thread *th = current_thread();
	extern void *_init_start;

	printf("Starting init\n");
	entry = elfld(_init_start);
	__usrentry_setup(&th->usrentry, entry);
	th->flags |= THFL_IN_USRENTRY;
	__insn_barrier();
	__usrentry_enter(th->usrentry.data);
	/* Not reached */
}

void test(void)
{
	int i;

	while (1) {
		if (i++ < 10)
			printf("Init started! %d\n", i);
		else
			i = 100;
		schedule();
	}
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

	//cpu_wakeup_aps();
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
