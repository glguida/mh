#include <microkernel.h>

extern void __sighdlr(void);

static __aligned(PAGE_SIZE) char _sigstack[PAGE_SIZE];

static int sighandler(int sig, unsigned long a1, unsigned long a2) 
{
	return 0;	
}

void
siginit(void)
{
	sys_xcptentry(__sighdlr, _sigstack + PAGE_SIZE);
}

int sys_sighandler(int, unsigned long, unsigned long) __weak_alias(sighandler);
