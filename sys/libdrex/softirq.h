#ifndef __drex_softirq_h
#define __drex_softirq_h

unsigned irqalloc(void);
void irqfree(unsigned);
int softirq_register(unsigned irq, void (*start)(void *), void *arg);

#endif
