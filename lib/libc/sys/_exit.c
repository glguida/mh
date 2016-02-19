#include <sys/cdefs.h>
#include <microkernel.h>
#include <sys/wait.h>

__strong_alias(_Exit, _exit);

__dead void _exit(int status)
{
	sys_die(W_EXITCODE(status, 0));
}
