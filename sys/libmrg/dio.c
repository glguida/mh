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


DEVICE *
dopen(char *devname, devmode_t mode)
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


void
dclose(DEVICE *d)
{
	sys_close(d->dd);
	free(d);
}
