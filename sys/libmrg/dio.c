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
#define FLAG_HAS_EIO 1
	unsigned flags;
	int dd;

	/* EIO: kernel end of I/O interrupt. received if devices needs E/IO. */
	int eio;

	/* DIO: Events to set at the end of the DIO operation. */
	int dioevt;
};

static void __eio_handler(int eioint, void *arg)
{
	DEVICE *d = arg;

	assert(d->eio != -1);
	__evtset(d->dioevt);
	d->dioevt = -1;
}

DEVICE *
dopen(char *devname, devmode_t mode)
{
	DEVICE *d;
	int ret, dd;
	uint64_t nameid;
	struct sys_rdcfg_cfg cfg;

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

	ret = sys_rdcfg(dd, &cfg);
	if (ret < 0) {
		perror("rdcfg(%s)", devname);
		sys_close(dd);
		free(d);
		return NULL;
	}

	if (cfg.eio != (uint8_t)-1) {
		int eio = intalloc();

		ret = sys_mapirq(dd, cfg.eio, eio);
		if (ret < 0) {
			perror("mapirq(%s, %d, %d)",
			       devname, cfg.eio, eio);
			sys_close(dd);
			free(d);
			return NULL;
		}

		inthandler(eio, __eio_handler, (void *)d);
		d->flags |= FLAG_HAS_EIO;
		d->eio = eio;
	} else {
		d->eio = -1;
	}
	d->dioevt = -1;

	printf("Opened device %p (%d): %d -> %d (flags: %lx)\n",
	       d, d->dd, cfg.eio, d->eio, d->flags);

	return d;
}

void
dwait(DEVICE *d)
{
	preempt_disable();
	if (d->dioevt != -1)
		evtwait(d->dioevt);
	preempt_enable();
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
vdio(DEVICE *d, int evtid, enum dio_op op, va_list opargs)
{
	int ret, w_eio = d->eio != -1;

	if (w_eio) {
		/* EIO wait needed */
		preempt_disable();
		if (d->dioevt != -1) {
			preempt_enable();
			return -EBUSY;
		}
		d->dioevt = evtid;
		preempt_enable();
	}

	switch (op) {
	case PORT_IN: {
		uint64_t port = va_arg(opargs, uint64_t);
		uint64_t val;

		ret = sys_in(d->dd, port, &val);
		if (ret != 0)
			break;

		if (!w_eio) {
			/* IN is synchronous */
			preempt_disable();
			__evtset(d->dioevt);
			preempt_enable();
		}
		break;
	}
	case PORT_OUT: {
		uint64_t port = va_arg(opargs, uint64_t);
		uint64_t val = va_arg(opargs, uint64_t);

		ret = sys_out(d->dd, port, val);
		if (ret != 0)
			break;

		if (!w_eio) {
			/* OUT is synchronous */
			preempt_disable();
			__evtset(d->dioevt);
			preempt_enable();
		}
		break;
	}
	}

	if (ret != 0) {
		/* Wake up if someone has been set up to wait for this
		 * event. Please  note that  this means the  the event
		 * listeners must be prepared for spurious events! */
		preempt_disable();
		__evtset(d->dioevt);
		d->dioevt = -1;
	}

	return ret;
}

int
dio(DEVICE *d, int evtid, enum dio_op op, ...)
{
	int ret;
	va_list opargs;

	va_start(opargs, op);
	ret = vdio(d, evtid, op, opargs);
	va_end(opargs);

	return ret; 
}

int
diow(DEVICE *d, int evtid, enum dio_op op, ...)
{
	int ret;
	uint64_t *val;
	va_list opargs;

	va_start(opargs, op);

	/* Wait for previous events to finish */
	dwait(d);

	ret = vdio(d, evtid, op, opargs);
	if (ret != 0) {
		va_end(opargs);
		return ret;
	}

	/* Wait for current event to terminate */
	dwait(d);

	if (d->dioevt == -1) {
		va_arg(opargs, uint64_t);
		val = va_arg(opargs, uint64_t *);
		ret = sys_retval(d->dd, val);
		if (ret != 0) {
			va_end(opargs);
			return ret;
		}
	}

	va_end(opargs);
	return ret; 
}

int
dretval(DEVICE *d, uint64_t *val)
{

	return sys_retval(d->dd, val);
}

void
dclose(DEVICE *d)
{
	sys_close(d->dd);
	free(d);
}
