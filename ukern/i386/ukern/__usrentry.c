#include <uk/types.h>
#include <machine/uk/param.h>
#include <machine/uk/cpu.h>
#include <ukern/kern.h>
#include <lib/lib.h>

void __usrentry_enter(void *frame)
{
	void ___usrentry_enter(void *frame);

	current_cpu()->tss.esp0 =
		(uint32_t) current_thread()->stack_4k + 0xff0;
	current_cpu()->tss.ss0 = KDS;
	___usrentry_enter(frame);
}

void __usrentry_setxcpt(struct usrentry *ue, unsigned long xcpt,
			unsigned long arg1, unsigned long arg2)
{
	unsigned long *ptr = (unsigned long *) ue->data;

	/* eax: xcpt
	 * edi: arg1
	 * esi: arg2
	 */
	ptr[11] = xcpt;
	ptr[4] = arg1;
	ptr[5] = arg2;
}

void __usrentry_setup(struct usrentry *ue, vaddr_t ip, vaddr_t sp)
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
	ptr[16] = sp;		/* ESP */
	ptr[17] = UDS;		/* SS */
}

void __usrentry_save(struct usrentry *ue, void *frame)
{
	memcpy(ue->data, frame, sizeof(ue->data));
}
