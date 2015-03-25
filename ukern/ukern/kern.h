#ifndef _ukern_kern_h
#define _ukern_kern_h

#include <uk/types.h>
#include <uk/queue.h>
#include <machine/uk/pmap.h>
#include <uk/sys.h>

#define THST_RUNNING 0
#define THST_RUNNABLE 1
#define THST_STOPPED 2
#define THST_DELETED 3

#define _THFL_IN_USRENTRY 0
#define THFL_IN_USRENTRY (1 << _THFL_IN_USRENTRY)
#define _THFL_IN_XCPTENTRY 1
#define THFL_IN_XCPTENTRY (1 << _THFL_IN_XCPTENTRY)
#define _THFL_XCPTENTRY 2
#define THFL_XCPTENTRY (1 << _THFL_XCPTENTRY)

#define thread_is_idle(_th) (_th == current_cpu()->idle_thread)

struct thread {
	jmp_buf ctx;
	struct pmap *pmap;

	struct xcptframe usrentry;
	struct xcptframe xcptentry;
	void *stack_4k;
	void *frame;
	void *xcptframe;

	uint16_t flags;
	uint16_t status;
	 TAILQ_ENTRY(thread) sched_list;
};

extern int usrpgfault;
int wruser(void *, void *, size_t);

int thxcpt(unsigned xcpt);
void kern_boot(void);
void kern_bootap(void);

void cpu_softirq_raise(int);
void do_cpu_softirq(void);

void schedule(void);
void die(void) __dead;

#endif
