#ifndef _ukern_thread_h
#define _ukern_thread_h

#include <uk/types.h>
#include <uk/queue.h>
#include <machine/uk/pmap.h>

#define _THFL_STOPPED 0
#define THFL_STOPPED (1 << _THFL_STOPPED)

#define _THFL_THEPOFF 1
#define THFL_THEPOFF (1 << _THFL_THEPOFF)
#define _THFL_THEPLD  2
#define THFL_THEPLD  (1 << _THFL_THEPLD)
#define _THFL_THEPON  3
#define THFL_THEPON  (1 << _THFL_THEPON)

#define thread_is_runnable(_th) (!((_th)->flags & THFL_STOPPED))
#define thread_is_idle(_th) (_th == current_cpu()->idle_thread)

struct thread {
    jmp_buf        ctx;
    struct addrspc *addrspc;
    void           *stack_4k;

    struct thep    *current;

    uint32_t            flags;
    TAILQ_ENTRY(thread) sched_list;
};

void thread_init(void);
struct thread *thread_boot(struct addrspc *);
struct thread *thread_create(struct addrspc *, void (*)(void));
void thread_switch(struct thread *);

#endif
