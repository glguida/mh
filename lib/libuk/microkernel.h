#ifndef _microkernel_h_
#define _microkernel_h_

#include <uk/sys.h>
#include <uk/types.h>
#include <machine/uk/vmparam.h>

#ifndef __dead
#define __dead __attribute__((__noreturn__))
#endif

int sys_xcptentry(void (*)(void), void *);
__dead void sys_die();

int sys_putc(int ch);

extern int sys_sighandler(int, unsigned long, unsigned long);

#endif
