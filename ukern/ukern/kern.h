#ifndef _ukern_kern_h
#define _ukern_kern_h

#include <uk/types.h>
#include <uk/queue.h>
#include <machine/uk/pmap.h>

void __usrentry_setup(struct usrentry *ue, vaddr_t ip, vaddr_t sp);
void __usrentry_enter(void *frame);

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

	void *stack_4k, *frame;
	struct usrentry usrentry;
	struct usrentry xcptentry;

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
