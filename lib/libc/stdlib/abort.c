#include <sys/cdefs.h>

extern void (*__cleanup)(void);
static int aborting = 0;

void
abort(void)
{
	if (!aborting) {
		aborting = 1;

		if (__cleanup)
			(*__cleanup)();
	}
	_exit(1);
}
