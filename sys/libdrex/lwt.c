#include <assert.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <setjmp.h>
#include <stdlib.h>
#include <drex/lwt.h>
#include <syslib.h>
#include <microkernel.h>

static int lwt_initialized = 0;
static lwt_t __lwt_main;
static lwt_t *lwt_current = NULL;

SIMPLEQ_HEAD(, lwt) __lwtq_active = SIMPLEQ_HEAD_INITIALIZER(__lwtq_active);

static void
lwt_init(void)
{
	assert(!lwt_initialized);
	__lwt_main.priv = NULL;
	lwt_current = &__lwt_main;
	lwt_current->active = 0;
	lwt_initialized++;
}

void *
lwt_getprivate(void)
{

	if (lwt_current == NULL)
		return NULL;

	return lwt_current->priv;
}

void
lwt_setprivate(void *ptr)
{

	if (!lwt_initialized)
		lwt_init();
	lwt_current = ptr;
}

lwt_t *
lwt_getcurrent(void)
{
	if (!lwt_initialized)
		lwt_init();
	return lwt_current;
}

static void
_lwt_switch(lwt_t *new, unsigned save)
{
	lwt_t *old;

	old = lwt_current;
	lwt_current = new;
	if (!save || !_setjmp(old->buf))
		_longjmp(new->buf, 1);
}

void
__lwt_wake(lwt_t *lwt)
{
	/* Interrupt context, no cli necessary */
	if (!lwt_initialized)
		lwt_init();

  	if (!lwt->active) {
		SIMPLEQ_INSERT_TAIL(&__lwtq_active, lwt, list);
		lwt->active = 1;
	}
}

void
lwt_wake(lwt_t *lwt)
{
	if (!lwt_initialized)
		lwt_init();

	assert(lwt != lwt_current);
	sys_cli();
	if (!lwt->active) {
		SIMPLEQ_INSERT_TAIL(&__lwtq_active, lwt, list);
		lwt->active = 1;
	}
	sys_sti();
}

void
lwt_yield(void)
{
	lwt_t *old, *new;

	if (!lwt_initialized)
		lwt_init();
	old = lwt_current;

	sys_cli();
	SIMPLEQ_INSERT_TAIL(&__lwtq_active, old, list);
	old->active = 1;
	new = SIMPLEQ_FIRST(&__lwtq_active);
	SIMPLEQ_REMOVE_HEAD(&__lwtq_active, list);
	new->active = 0;
	sys_sti();
	if (old != new)
		_lwt_switch(new, 1);
}

void
lwt_sleep(void)
{
	lwt_t *new;

	if (!lwt_initialized)
		lwt_init();
	sys_cli();
	lwt_current->active = 0;
retry:
	new = SIMPLEQ_FIRST(&__lwtq_active);
	if (new != NULL) {
		SIMPLEQ_REMOVE_HEAD(&__lwtq_active, list);
		new->active = 0;
	}
	while (new == NULL) {
		sys_wait(); /* Implicit sti() */
		sys_cli();
		goto retry;
	}
	sys_sti();	
	_lwt_switch(new, 1);
}

void
lwt_exit(void)
{
	lwt_t *old, *new;

	if (!lwt_initialized) {
		printf("die: lwt_exit() on single LWT system\n");
		sys_die();
	}

	old = lwt_current;

	sys_cli();
	old->active = 0;
retry:
	new = SIMPLEQ_FIRST(&__lwtq_active);
	if (new != NULL) {
		SIMPLEQ_REMOVE_HEAD(&__lwtq_active, list);
		new->active = 0;
	}
	while (new == NULL) {
		sys_wait();
		goto retry;
	}
	sys_sti();

	/* Reset LWT to starting point */
	lwt_makebuf(old, old->start, old->arg, old->stack, old->stack_size);

	_lwt_switch(new, 0);
}

lwt_t *
lwt_create(void (*start)(void *), void *arg, size_t stack_size)
{
	lwt_t *lwt;

	lwt = malloc(sizeof(stack_size) + sizeof(lwt_t));
	if (lwt == NULL)
		return NULL;

	lwt->active = 0;
	lwt->stack = (void *)(lwt + 1);
	lwt->stack_size = stack_size;
	lwt->start = start;
	lwt->arg = arg;
	lwt->priv = NULL;

	lwt_makebuf(lwt, start, arg, (void *)(lwt+1), stack_size);
	return lwt;
}
