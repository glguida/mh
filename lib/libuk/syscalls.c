#include <microkernel.h>
#include "__sys.h"

__dead void die(void)
{
	int dummy;

	__syscall0(SYS_DIE, dummy);
	/* Not reached */
}

int sys_putc(int ch)
{
	int ret;

	__syscall1(SYS_PUTC, ch, ret);
	return ret;
}

int sys_xcptentry(void (*func) (void), void *frame, void *stack)
{
	int ret;

	__syscall3(SYS_XCPTENTRY, (unsigned long) func,
		   (unsigned long) frame, (unsigned long) stack, ret);
	return ret;
}

int __dead sys_xcptreturn(int ret)
{
	int dummy;

	__syscall1(SYS_XCPTRET, ret, dummy);
	/* Not reached */
}
