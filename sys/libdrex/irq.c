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
#include <drex/event.h>


/*
 * IRQ queues and interrupts.
 */

unsigned __preemption_level = 0;
static uint64_t free_irqs = -1;
static int __dirtio_dev_irq = -1;
static struct drex_events irq_events[MAX_EXTINTRS] = { {0, }, };


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
	return _drex_kqueue_events(&irq_events[irq], EVFILT_DREX_IRQ, 0);
}

static int irq_queue_attach(struct drex_event *e, uintptr_t irq)
{
	if (irq >= MAX_EXTINTRS)
		return -EINVAL;

	preempt_disable();
	LIST_INSERT_HEAD(&irq_events[irq], e, event_list);
	preempt_enable();
	return 0;
}

static int irq_queue_detach(struct drex_event *e, uintptr_t irq)
{
	if (irq >= MAX_EXTINTRS)
		return -EINVAL;

	preempt_disable();
	LIST_REMOVE(e, event_list);
	preempt_enable();
	return 0;
}

struct evfilter _irq_evfilter = {
	.attach = irq_queue_attach,
	.detach = irq_queue_detach,
	.event = NULL,
};

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


