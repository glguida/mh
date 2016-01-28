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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/bitops.h>
#include <sys/errno.h>
#include <syslib.h>
#include <drex/lwt.h>
#include <drex/preempt.h>
#include <microkernel.h>

unsigned __preemption_level = 0;

static uint64_t free_irqs = -1;
static lwt_t *__softirq[MAX_EXTINTRS] = {0, };
static int __dirtio_dev_irq = -1;

void __dirtio_dev_process(void);

int __sys_inthandler(int prio, uint64_t si, struct intframe *f)
{
	unsigned irq;

	while (si != 0) {
		irq = ffs64(si) - 1;
		si &= ~((uint64_t)1 << irq);

		printf("Waking up IRQ %d\n", irq);
		if (__softirq[irq]) {
			printf("okay!\n");
			__lwt_wake(__softirq[irq]);
		}
		if ((__dirtio_dev_irq != -1) && (__dirtio_dev_irq == irq))
			__dirtio_dev_process();
	}
	return 0;
}

void
irq_set_dirtio(unsigned irq)
{
	__dirtio_dev_irq = irq;
}

int
softirq_register(unsigned irq, void (*start)(void *), void *arg)
{
	lwt_t *olwt, *nlwt;
	
	if (irq >= MAX_EXTINTRS)
		return -EINVAL;
	nlwt = lwt_create(start, arg, 65536);
	assert(nlwt != NULL);
	preempt_disable();
	olwt = __softirq[irq];
	__softirq[irq] = nlwt;
	preempt_enable();
	free(olwt);
	return 0;
}

void
irqwait(unsigned irq)
{

	assert(irq < 64);

	preempt_disable();
	assert(__softirq[irq] == NULL);
	__softirq[irq] = lwt_getcurrent();
	lwt_sleep();
	__softirq[irq] = NULL;
	preempt_enable();
	
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
