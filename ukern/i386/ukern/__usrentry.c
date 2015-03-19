#include <uk/types.h>
#include <machine/uk/param.h>
#include <machine/uk/cpu.h>
#include <ukern/thread.h>
#include <ukern/usrentry.h>
#include <lib/lib.h>

void __usrentry_enter(void *frame)
{
	void ___usrentry_enter(void *frame);

	current_cpu()->tss.esp0 =
		(uint32_t) current_thread()->stack_4k + 0xff0;
	current_cpu()->tss.ss0 = KDS;
	___usrentry_enter(frame);
}

void __usrentry_setup(struct usrentry *ue, vaddr_t ip)
{
	unsigned long *ptr = (unsigned long *) ue->data;

	memset(ue->data, 0, 17 * 4);
	ptr[0] = (UDS << 16) | UDS;	/* DS, ES */
	ptr[1] = (UDS << 16) | UDS;	/* FS, GS */

	/*
	 * ptr[2]: cr3;
	 * ptr[3]: cr2;
	 * ptr[4]: edi;
	 * ptr[5]: esi;
	 * ptr[6]: ebp;
	 * ptr[7]: esp (ign);
	 * ptr[8]: ebx;
	 * ptr[9]: edx;
	 * ptr[10]: ecx;
	 * ptr[11]: eax;
	 * ptr[12]: error (fake);
	 */

	/* IRET FRAME */
	ptr[13] = ip;		/* EIP */
	ptr[14] = UCS;		/* CS */
	ptr[15] = 0x202;	/* EFLAGS */
	ptr[16] = 0;		/* ESP */
	ptr[17] = UDS;		/* SS */
}
