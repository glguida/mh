#ifndef _microkernel_h_
#define _microkernel_h_

#include <uk/sys.h>
#include <uk/types.h>

#ifndef __dead
#define __dead __attribute__((__noreturn__))
#endif

int sys_xcptentry(unsigned long ip, unsigned long stk);
__dead void sys_die();

int sys_putc(int ch);

#endif
