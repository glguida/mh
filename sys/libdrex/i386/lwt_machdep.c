#include <sys/types.h>
#include <setjmp.h>
#include <drex/lwt.h>

void _setupjmp(jmp_buf, void *, void *);

void
lwt_create(lwt_t *lwt, void (*start)(void *), void * arg,
	   void *priv, void *stack_base, size_t stack_size)
{
	void *sp = (stack_base + stack_size - sizeof(void *));

	*(void **)sp = arg;
	_setupjmp(lwt->buf, (void *)start, sp);
	lwt->priv = priv;
}
