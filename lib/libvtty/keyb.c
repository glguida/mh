#include <assert.h>
#include <stdlib.h>
#include <sys/queue.h>

#define CLIST_SIZE 64
struct clist {
	unsigned read, written;
	int buf[CLIST_SIZE];
	TAILQ_ENTRY(clist) cle;
};
static TAILQ_HEAD(clisthead, clist) clist = TAILQ_HEAD_INITIALIZER(clist);

void _clist_add(int c)
{
	struct clist *cl;

	cl = TAILQ_LAST(&clist,clisthead);
	if (cl == NULL) {
		cl = malloc(sizeof(*cl));
		cl->read = 0;
		cl->written = 0;
		cl->buf[cl->written++] = c;
		TAILQ_INSERT_HEAD(&clist, cl, cle);
		return;
	}
	if (cl->written >= CLIST_SIZE) {
		struct clist *cn;

		cn = malloc(sizeof(*cn));
		cn->read = 0;
		cn->written = 0;
		cn->buf[cl->written++] = c;
		TAILQ_INSERT_AFTER(&clist, cl, cn, cle);
		return;
	}
	cl->buf[cl->written++] = c;
}

static int clist_fetch(void)
{
	int c = 0;
	struct clist *cl;

	cl = TAILQ_FIRST(&clist);
	if (cl == NULL)
		return c;

	assert(cl->written <= CLIST_SIZE);
	assert(cl->read <= cl->written);

	if (cl->read < cl->written)
		c = cl->buf[cl->read++];

	if (cl->read == cl->written) {
		TAILQ_REMOVE(&clist, cl, cle);
		free(cl);
	}

	return c;
}

int vtty_kgetc(void)
{

	return clist_fetch();
}

int vtty_kwait(void)
{

	vtdrv_kwait();
}

int vtty_kgetcw(void)
{
	int c;

	c = vtty_kgetc();
	if (!c) {
		vtty_kwait();
		c = vtty_kgetc();
	}
	return c;
}
