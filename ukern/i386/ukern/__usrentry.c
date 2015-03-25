#include <uk/types.h>
#include <machine/uk/param.h>
#include <machine/uk/cpu.h>
#include <ukern/kern.h>
#include <lib/lib.h>

void __xcptframe_enter(struct xcptframe *f)
{
	current_cpu()->tss.esp0 =
		(uint32_t) current_thread()->stack_4k + 0xff0;
	current_cpu()->tss.ss0 = KDS;
	___usrentry_enter((void *) f);
}

void __xcptframe_setxcpt(struct xcptframe *f, unsigned long xcpt)
{
	f->eax = xcpt;
}


int __xcptframe_usrupdate(struct xcptframe *f, struct xcptframe *usrf)
{
	int ret;

	ret = usercpy(&f->edi, &usrf->edi, 40 /* edi to eip */ );
	if (ret)
		return -1;

	ret = usercpy(&f->esp, &usrf->esp, 4);
	if (ret)
		return -1;

	return 0;
}

void __xcptframe_setup(struct xcptframe *f, vaddr_t ip, vaddr_t sp)
{
	memset(f, 0, sizeof(*f));
	f->ds = UDS;
	f->es = UDS;
	f->fs = UDS;
	f->gs = UDS;
	f->eip = ip;
	f->cs = UCS;
	f->eflags = 0x202;
	f->esp = sp;
	f->ss = UDS;
}

void __usrentry_save(struct xcptframe *f, void *ptr)
{
	memcpy(f, ptr, sizeof(*f));
}
