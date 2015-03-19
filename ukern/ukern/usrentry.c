#include <uk/types.h>
#include <machine/uk/cpu.h>
#include <ukern/thread.h>
#include <ukern/usrentry.h>

void usrentry_setup(struct usrentry *ue, vaddr_t ip)
{

	__usrentry_setup(ue, ip);
}

void usrentry_enter(struct usrentry *ue)
{
	struct thread *th = current_thread();

	th->usrentry = ue;
	th->flags |= THFL_IN_USRENTRY;
	__insn_barrier();
	__usrentry_enter(ue->data);
	/* Not reached */
}

struct usrentry *usrentry_leave(void)
{
	struct usrentry *ue;
	struct thread *th = current_thread();

	ue = th->usrentry;
	memcpy(ue->data, th->frame, sizeof(ue->data));
	th->usrentry = NULL;
	th->flags &= ~THFL_IN_USRENTRY;
	return ue;
}
