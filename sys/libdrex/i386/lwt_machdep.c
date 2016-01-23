#include <sys/types.h>
#include <setjmp.h>
#include <drex/lwt.h>

void _setupjmp(jmp_buf, void *, void *);

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
