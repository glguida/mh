#include <machine/uk/cpu.h>
#include <uk/sys.h>
#include <ukern/kern.h>
#include <lib/lib.h>

int
sys_putc(int ch)
{
	sysputc(ch);
	return 0;
}

int
sys_die(void)
{
	die();
	return 0;
}

int
sys_xcptentry(vaddr_t entry, vaddr_t stack)
{
	struct thread *th = current_thread();

	__usrentry_setup(&th->xcptentry, entry, stack);
	th->flags |= THFL_XCPTENTRY;
	return 0;	
}


