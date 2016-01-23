#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/bitops.h>
#include <sys/errno.h>
#include <syslib.h>
#include <drex/lwt.h>
#include <microkernel.h>

static uint64_t free_irqs = -1;
static lwt_t *__softirq[MAX_EXTINTRS] = {0, };

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
	}
	return 0;
}

int
softirq_register(unsigned irq, void (*start)(void *), void *arg)
{
	lwt_t *olwt, *nlwt;
	
	if (irq >= MAX_EXTINTRS)
		return -EINVAL;
	nlwt = lwt_create(start, arg, 1024);
	assert(nlwt != NULL);
	sys_cli();
	olwt = __softirq[irq];
	__softirq[irq] = nlwt;
	sys_sti();
	free(olwt);
	return 0;
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
	assert(irq <= 64);
	free_irqs |= ((uint64_t)1 << irq);
}
