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



#include <sys/null.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/errno.h>
#include <sys/bitops.h>
#include <microkernel.h>
#include <stdlib.h>
#include <assert.h>
#include <syslib.h>

#include <drex/preempt.h>
#include <drex/lwt.h>


#define DREX_QUEUES_MAX 16

struct drex_queue {
#define DREX_QUEUE_LWTWAIT 1
	unsigned flags;
	lwt_t *lwt;
	LIST_HEAD(, drex_event) events;
};

struct drex_event {
	unsigned irq;
	int active;

	struct drex_queue *queue;
	LIST_ENTRY(drex_event) irq_list;
	LIST_ENTRY(drex_event) queue_list;
};
static struct drex_queue *queues[DREX_QUEUES_MAX] = { 0, };

/*
 * IRQ queues and interrupts.
 */

unsigned __preemption_level = 0;
static uint64_t free_irqs = -1;
static int __dirtio_dev_irq = -1;
static LIST_HEAD(,drex_event) irq_events[MAX_EXTINTRS] = { {0, }, };


void __dirtio_dev_process(void);
static int __irq_queue_event(int irq);

int __sys_inthandler(int prio, uint64_t si, struct intframe *f)
{
	int cnt;
	unsigned irq;

	while (si != 0) {
		irq = ffs64(si) - 1;
		si &= ~((uint64_t)1 << irq);

		printf("Sending event to IRQ %d\n", irq);
		cnt = __irq_queue_event(irq);

		if ((__dirtio_dev_irq != -1) && (__dirtio_dev_irq == irq))
			__dirtio_dev_process();
		else if (cnt == 0) {
			printf("IRQ%d: ignored\n");
		}
	}
	return 0;
}

static int __irq_queue_event(int irq)
{
	/* Interrupt context */
	int cnt = 0;
	struct drex_event *e;

	assert(irq <= MAX_EXTINTRS);
	LIST_FOREACH(e, &irq_events[irq], irq_list) {
		if (e->queue->lwt != NULL) {
			__lwt_wake(e->queue->lwt);
			e->queue->lwt = NULL;
		}
		e->active = 1;
		cnt++;
	}
	return cnt;
}

static void irq_queue_register(int irq, struct drex_event *e)
{
	assert(irq <= MAX_EXTINTRS);

	preempt_disable();
	LIST_INSERT_HEAD(&irq_events[irq], e, irq_list);
	preempt_enable();
}

/*
 * A random act of IRQ allocator.
 */

void
irq_set_dirtio(unsigned irq)
{
	__dirtio_dev_irq = irq;
}

unsigned
irqalloc(void)
{
	unsigned irq;

	assert(free_irqs != 0);
	irq = ffs64(free_irqs) - 1;
	free_irqs &= ~((uint64_t)1 << irq);
	return irq;
}

void
irqfree(unsigned irq)
{
	assert(irq < 64);
	free_irqs |= ((uint64_t)1 << irq);
}


/*
 * IRQ Queues.
 */


int
drex_kqueue(void)
{
	int i;
	struct drex_queue *ptr;

	for (i = 0; i < DREX_QUEUES_MAX; i++)
		if (queues[i] == NULL)
			break;

	if (i >= DREX_QUEUES_MAX)
		return -EBUSY;

	ptr = malloc(sizeof(struct drex_queue));
	if (ptr == NULL)
		return -ENOMEM;

	ptr->flags = 0;
	ptr->lwt = NULL;
	LIST_INIT(&ptr->events);

	queues[i] = ptr;
	return i;
}

int
drex_kqueue_add_irq(int qn, unsigned irq)
{
	struct drex_queue *q;
	struct drex_event *e;

	if (qn >= DREX_QUEUES_MAX)
		return -EINVAL;

	q = queues[qn];
	if (q == NULL)
		return -ENOENT;

	if (irq >= MAX_EXTINTRS)
		return -EINVAL;

	/* Check event already registered */
	LIST_FOREACH(e, &q->events, queue_list)
		if (e->irq == irq)
			return 0;

	e = malloc(sizeof(*e));
	if (e == NULL)
		return -ENOMEM;

	e->irq = irq;
	e->active = 0;
	e->queue = q;
	irq_queue_register(irq, e);
	LIST_INSERT_HEAD(&q->events, e, queue_list);
	return 0;
}

int
drex_kqueue_del_irq(int qn, unsigned irq)
{
	return -ENOSYS;
}

int
drex_kqueue_wait(int qn)
{
	struct drex_queue *q;
	struct drex_event *e;

	if (qn >= DREX_QUEUES_MAX)
		return -EINVAL;

	q = queues[qn];
	if (q == NULL)
		return -ENOENT;

	assert(!q->flags & DREX_QUEUE_LWTWAIT);
	preempt_disable();
	while (1) {
		LIST_FOREACH(e, &q->events, queue_list) {
			if (e->active) {
				e->active = 0;
				preempt_enable();
				return e->irq;
			}
		}
		/* No events. Wait */
		q->lwt = lwt_getcurrent();
		lwt_sleep();
	}
}
