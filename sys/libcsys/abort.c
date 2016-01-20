#include <microkernel.h>

void
abort(void)
{
	sys_die();
}
