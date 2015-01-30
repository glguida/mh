#include <uk/types.h>
#include <uk/param.h>
#include <ukern/pfndb.h>
#include <ukern/fixmems.h>
#include <ukern/heap.h>
#include <ukern/vmap.h>
#include <ukern/thread.h>
#include <machine/uk/pmap.h>
#include <machine/uk/cpu.h>
#include <machine/uk/locks.h>
#include <machine/uk/platform.h>
#include <lib/lib.h>
#include "kern.h"

static lock_t sched_lock = 0;
static TAILQ_HEAD(thread_list, thread) running_threads = \
    TAILQ_HEAD_INITIALIZER(running_threads);

lock_t softirq_lock = 0;
uint64_t softirqs = 0;
#define SOFTIRQ_RESCHED (1 << 0)

void
cpu_softirq_raise(int id)
{

    printf("Raising softirq %d\n", id);
    current_cpu()->softirq |= id;
}

void
do_cpu_softirq(void)
{
    uint64_t si;
    struct thread *th, *tmp;

    while (current_cpu()->softirq) {
	si = current_cpu()->softirq;
	current_cpu()->softirq = 0;

	if (si & SOFTIRQ_RESCHED) {
	    printf("resched!\n");
	    TAILQ_FOREACH_SAFE(th, &current_cpu()->resched, sched_list, tmp) {
		TAILQ_REMOVE(&current_cpu()->resched, th, sched_list);
		spinlock(&sched_lock);
		printf("Adding back %p\n", th);
		TAILQ_INSERT_TAIL(&running_threads, th, sched_list);
		spinunlock(&sched_lock);
	    }
	}
    }
}

void
schedule(void)
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

    thread_switch(th);
}

void
idle(void)
{
    while(1) {
	schedule();
	do_cpu_softirq();
	platform_wait();
    }
}

void
test(void)
{
    int i;

    while(1) {
	if (i++ < 10)
	    printf("Init started! %d\n", i);
	else
	    i = 100;
	schedule();
    }
}

void
kern_boot(void)
{
  struct thread *th;

    printf("Kernel loaded at va %08lx:%08lx\n", UKERNTEXTOFF, UKERNEND);
    //pfndb_printranges();
    //pfndb_printstats();

    th = thread_create(current_thread()->addrspc, test);
    spinlock(&sched_lock);
    TAILQ_INSERT_TAIL(&running_threads, th, sched_list);
    spinunlock(&sched_lock);
    printf("Adding thread %p\n", th);

    cpu_wakeup_aps();
    idle();
}

void
kern_bootap(void)
{

    idle();
}
