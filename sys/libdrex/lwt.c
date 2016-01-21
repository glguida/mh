#include <sys/null.h>
#include <assert.h>
#include <sys/types.h>
#include <drex/lwt.h>
#include <setjmp.h>

static int lwt_initialized = 0;
static lwt_t *lwt_current = NULL;

void *
lwt_getprivate(void)
{

	if (lwt_current == NULL)
		return NULL;

	return lwt_current->priv;
}

void
lwt_setprivate(void *ptr)
{

	lwt_current = ptr;
}

lwt_t *
lwt_getcurrent(void)
{
	return lwt_current;
}

void
lwt_init(lwt_t *lwt)
{
	assert(!lwt_initialized);
	lwt->priv = NULL;
	lwt_current = lwt;
	lwt_initialized++;
}

void
lwt_switch(lwt_t *lwt)
{
	lwt_t *old;

	old = lwt_current;
	lwt_current = lwt;
	if (!_setjmp(old->buf))
		_longjmp(lwt->buf, 1);
}
