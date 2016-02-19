/*
 * Copyright (c) 2015, Gianluca Guida
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <assert.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <setjmp.h>
#include <stdlib.h>
#include <drex/lwt.h>
#include <drex/preempt.h>


static int lwt_initialized = 0;
static lwt_t __lwt_main;
lwt_t *lwt_current = NULL;

SIMPLEQ_HEAD(, lwt) __lwtq_active = SIMPLEQ_HEAD_INITIALIZER(__lwtq_active);

static void
lwt_init(void)
{
	/* Can be called in interrupt context */
	assert(!lwt_initialized);
	__lwt_main.priv = NULL;
	lwt_current = &__lwt_main;
	lwt_current->flags = 0;
	lwt_initialized++;
}

void
lwt_exception_clear(void)
{
	lwt_t *lwt = lwt_getcurrent();
	lwt->flags &= ~LWTF_XCPT;
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

  	if (!(lwt->flags & LWTF_ACTIVE)) {
		SIMPLEQ_INSERT_TAIL(&__lwtq_active, lwt, list);
		lwt->flags |= LWTF_ACTIVE;
	}
}

void
lwt_wake(lwt_t *lwt)
{
	if (!lwt_initialized)
		lwt_init();

	assert(lwt != lwt_current);
	preempt_disable();
	if (!(lwt->flags & LWTF_ACTIVE)) {
		SIMPLEQ_INSERT_TAIL(&__lwtq_active, lwt, list);
		lwt->flags |= LWTF_ACTIVE;
	}
	preempt_enable();
}

void
lwt_yield(void)
{
	lwt_t *old, *new;

	if (!lwt_initialized)
		lwt_init();
	old = lwt_current;

	preempt_disable();
	SIMPLEQ_INSERT_TAIL(&__lwtq_active, old, list);
	old->flags |= LWTF_ACTIVE;
	new = SIMPLEQ_FIRST(&__lwtq_active);
	SIMPLEQ_REMOVE_HEAD(&__lwtq_active, list);
	new->flags &= ~LWTF_ACTIVE;
	preempt_enable();
	if (old != new)
		_lwt_switch(new, 1);
}

void
lwt_sleep(void)
{
	lwt_t *new;

	if (!lwt_initialized)
		lwt_init();
	preempt_disable();
	lwt_current->flags &= ~LWTF_ACTIVE;
retry:
	new = SIMPLEQ_FIRST(&__lwtq_active);
	if (new != NULL) {
		SIMPLEQ_REMOVE_HEAD(&__lwtq_active, list);
		new->flags &= ~LWTF_ACTIVE;
	}
	while (new == NULL) {
		sys_wait();
		preempt_restore();
		goto retry;
	}
	preempt_enable();
	_lwt_switch(new, 1);
}

void
lwt_exit(void)
{
	lwt_t *old, *new;

	if (!lwt_initialized) {
		printf("die: lwt_exit() on single LWT system\n");
		sys_die(1);
	}

	old = lwt_current;

	preempt_disable();
	old->flags &= ~LWTF_ACTIVE;
retry:
	new = SIMPLEQ_FIRST(&__lwtq_active);
	if (new != NULL) {
		SIMPLEQ_REMOVE_HEAD(&__lwtq_active, list);
		new->flags &= ~LWTF_ACTIVE;
	}
	while (new == NULL) {
		sys_wait();
		preempt_restore();
		goto retry;
	}
	preempt_enable();

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

	lwt->flags = 0;
	lwt->stack = (void *)(lwt + 1);
	lwt->stack_size = stack_size;
	lwt->start = start;
	lwt->arg = arg;
	lwt->priv = NULL;

	lwt_makebuf(lwt, start, arg, (void *)(lwt+1), stack_size);
	return lwt;
}
