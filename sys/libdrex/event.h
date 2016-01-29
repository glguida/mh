#ifndef __drex_event_h_
#define __drex_event_h_

unsigned irqalloc(void);
void irqfree(unsigned irq);


int drex_kqueue(void);
int drex_kqueue_add_irq(int qn, unsigned irq);
int drex_kqueue_del_irq(int qn, unsigned irq);
int drex_kqueue_wait(int qn);

#endif
