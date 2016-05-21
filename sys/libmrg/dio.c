#include <mrg.h>
#include <microkernel.h>
#include <squoze.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/errno.h>

#define perror(...) printf(__VA_ARGS__)
#warning add perror

struct _DEVICE {
	int dd;
	struct sys_rdcfg_cfg cfg;
};

static void __irq_handler(int irqint, void *arg)
{
	int irqevt = (int)(uintptr_t)arg;

	__evtset(irqevt);
}

DEVICE *
dopen(char *devname)
{
	DEVICE *d;
	int ret, dd;
	uint64_t nameid;

	d = malloc(sizeof(*d));
	if (d == NULL)
		return NULL;
	memset(d, 0, sizeof(*d));

	nameid = squoze(devname);
	dd = sys_open(nameid);
	if (dd < 0) {
		perror("open(%s)", devname);
		free(d);
		return NULL;
	}
	d->dd = dd;

	ret = sys_rdcfg(dd, &d->cfg);
	if (ret < 0) {
		perror("rdcfg(%s)", devname);
		sys_close(dd);
		free(d);
		return NULL;
	}

	return d;
}


/*
 * The plan:
 *
 * an event can set up any of:
 *
 * - AST. an AST is an softirq that can be raised (need raise
 *   syscall).  an AST will occupy only on interrupt line, and be
 *   handled in software.
 *
 * - ECB. A control block (size decided by the creator of the event,
 *   event filled by the setter).
 *
 * - Wake list. LWTs will wait on an event.
 */

int
din(DEVICE *d, uint32_t port, uint64_t *val)
{

	return sys_in(d->dd, port, val);
}


int
dout(DEVICE *d, uint32_t port, uint64_t val)
{

	return sys_out(d->dd, port, val);
}

int
dirq(DEVICE *d, unsigned irq, int evt)
{
	int ret, irqint;

	irqint = intalloc();
	ret = sys_mapirq(d->dd, irq, irqint);
	if (ret != 0) {
		intfree(irqint);
		return ret;
	}

	inthandler(irqint, __irq_handler, (void *)(uintptr_t)evt);
	return 0;
}

int
drdcfg(DEVICE *d, struct sys_rdcfg_cfg *cfg)
{

	return sys_rdcfg(d->dd, cfg);
}

void
dclose(DEVICE *d)
{
	sys_close(d->dd);
	free(d);
}
