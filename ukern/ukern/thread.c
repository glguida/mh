#include <uk/types.h>
#include <ukern/structs.h>
#include <ukern/fixmems.h>
#include <machine/uk/cpu.h>
#include "addrspc.h"
#include "thread.h"

static struct slab threads;

void
thread_init(void)
{

  setup_structcache(&threads, thread);
}

struct thread *
thread_boot(struct addrspc *as)
{
    struct thread *th;

    th = structs_alloc(&threads);
    th->addrspc = as;
    th->stack_4k = NULL; /* Statically allocated */
    set_current_thread(th);
    current_cpu()->idle_thread = th;
    return th;
}

void
thread_switch(struct thread *th)
{

    if (!_setjmp(current_thread()->ctx)) {
	addrspc_switch(th->addrspc);
	set_current_thread(th);
	_longjmp(th->ctx, 1);
	panic("WTF?\n");
    }
}

struct thread *
thread_create(struct addrspc *as, void (*__start)(void))
{
    struct thread *th;

    th = structs_alloc(&threads);
    th->flags = 0;
    th->addrspc = as;
    th->stack_4k = alloc4k();
    memset(th->stack_4k, 0, 4096);
    _setupjmp(th->ctx, __start, th->stack_4k + 0xff0);
    return th;
}

