#include <sys/types.h>
#include <microkernel.h>
#include <assert.h>

void
__assert13(const char *file, int line, const char *function,
    const char *failedexpr)
{
	(void)printf(
	    "assertion \"%s\" failed: file \"%s\", line %d%s%s%s\n",
	    failedexpr, file, line,
	    function ? ", function \"" : "",
	    function ? function : "",
	    function ? "\"" : "");
	sys_die();
	/* NOTREACHED */
}

