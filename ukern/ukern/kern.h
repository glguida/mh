#ifndef _ukern_kern_h
#define _ukern_kern_h

#include <uk/types.h>
#include <uk/queue.h>
#include <machine/uk/pmap.h>


#define THST_RUNNING 0
#define THST_RUNNABLE 1
#define THST_STOPPED 2
#define THST_DELETED 3

#define _THFL_STOPREQ 0
#define THFL_STOPREQ (1 << _THFL_STOPREQ)
#define _THFL_IN_USRENTRY 1
#define THFL_IN_USRENTRY (1 << _THFL_IN_USRENTRY)

#define thread_is_idle(_th) (_th == current_cpu()->idle_thread)

struct thread {
	jmp_buf ctx;
	struct pmap *pmap;

	void *stack_4k, *frame;
	struct usrentry usrentry;

	uint16_t flags;
	uint16_t status;
	 TAILQ_ENTRY(thread) sched_list;
};

int thpgfault(vaddr_t, unsigned long);
void kern_boot(void);
void kern_bootap(void);

void cpu_softirq_raise(int);
void do_cpu_softirq(void);

void schedule(void);
void die(void);

#endif
