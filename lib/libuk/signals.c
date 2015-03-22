#include <uk/types.h>
#include <microkernel.h>

extern void __sighdlr(void);

static __aligned(PAGE_SIZE)
char _sigstack[PAGE_SIZE];

static int sighandler(int sig, struct usrentry frame)
{
	return -1;
}

void siginit(void)
{
	void *stkptr =
		(void *) (_sigstack + PAGE_SIZE - sizeof(struct usrentry));
	sys_xcptentry(__sighdlr, stkptr, stkptr);
}

int sys_sighandler(int, struct usrentry frame) __weak_alias(sighandler);
