#include <sys/types.h>
#include <microkernel.h>
#include <setjmp.h>
#include <syslib.h>
#include <drex/lwt.h>

void _setupjmp(jmp_buf, void *, void *);

void framedump(struct intframe *f)
{

	printf("EIP: %x\tEFLAGS: %x\n", f->eip, f->eflags);
	printf("\tEAX: %x\tEBX: %x\tECX: %x\tEDX:%x\n",
	       f->eax, f->ebx, f->ecx, f->edx);
	printf("\tEDI: %x\tESI: %x\tEBP: %x\tESP:%x\n",
	       f->edi, f->esi, f->ebp, f->esp);
	printf("\tDS: %x\tES: %x\tFS: %x\tGS: %x\n",
	       f->ds, f->es, f->fs, f->gs);
}

void framelongjmp(volatile struct intframe *f, jmp_buf *jb)
{
	uint32_t *buf = (uint32_t *)(*jb);

	f->eip = buf[0];
	f->ebx = buf[1];
	f->esp = buf[2] + 4; /* ret consumes the stack */
	f->ebp = buf[3];
	f->esi = buf[4];
	f->edi = buf[5];
	f->eax = 1;
}

void
lwt_makebuf(lwt_t *lwt, void (*start)(void *), void * arg,
	    void *stack_base, size_t stack_size)
{
	void *sp = (void *)(stack_base + stack_size - 12);

	*(void **)(sp + 8) = arg;
	*(void **)(sp + 4) = 0x50500505; /* Canary for return value */
	*(void **)sp = 0; /* _longjmp uses ret, which consumes this */
	_setupjmp(lwt->buf, (void *)start, sp);
}
