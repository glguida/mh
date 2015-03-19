#ifndef _ukern_thread_h
#define _ukern_thread_h

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

#define thread_is_runnable(_th) ((_th)->status == THST_STOPPED)
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

#if 0
void thread_init(void);
struct thread *thread_boot(struct pmap *);
struct thread *thread_create(void (*)(void), struct image *);
void thread_switch(struct thread *);
#endif

#endif
