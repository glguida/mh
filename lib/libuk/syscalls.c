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
