#ifndef _microkernel_h_
#define _microkernel_h_

#include <sys/types.h>
#include <uk/sys.h>

int sys_xcptentry(void (*)(void), void *, void *);
__dead void sys_die();

int sys_putc(int ch);

extern int sys_sighandler(int, struct xcptframe frame);

#endif
