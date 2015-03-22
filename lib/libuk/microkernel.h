#ifndef _microkernel_h_
#define _microkernel_h_

#include <uk/sys.h>
#include <uk/types.h>
#include <machine/uk/vmparam.h>

int sys_xcptentry(void (*)(void), void *, void *);
__dead void sys_die();

int sys_putc(int ch);

extern int sys_sighandler(int, struct usrentry frame);

#endif
