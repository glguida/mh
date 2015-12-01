/*
 * Copyright (c) 2015, Gianluca Guida
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <uk/types.h>
#include <uk/bus.h>
#include <uk/structs.h>
#include <uk/kern.h>
#include <uk/usrdev.h>
#include <machine/uk/cpu.h>
#include <lib/lib.h>

#define MAXUSRDEVREMS 256

static struct slab usrdevs;
static struct slab usrioreqs;

struct usrioreq {
	unsigned id;
	uint64_t port;
	uint64_t val;

	TAILQ_ENTRY(usrioreq) queue;
};

struct remth {
	int use :1;			/* Entry in use (proc) */
	int bsy :1;			/* Request in progress (device) */
	unsigned id;			/* Remote ID descriptor */
	struct thread *th;		/* Remote threads */
	unsigned intrs[INTMAPSZ];	/* Interrupt Remapping Table */
};

struct usrdev {
	lock_t lock;
	struct dev dev;    /* Registered device */
	struct thread *th; /* Device thread */
	unsigned sig;      /* Request Signal */

	struct remth remths[MAXUSRDEVREMS];

	TAILQ_HEAD(,usrioreq) ioreqs;
};

struct usrdev_procopq {
	uint32_t id;
};

static unsigned _usrdev_open(void *devopq, uint64_t did)
{
	int i, ret = -1;
	struct usrdev *ud = (struct usrdev *)devopq;

	/* Current thread: Process */

	spinlock(&ud->lock);
	for (i = 0; i < MAXUSRDEVREMS; i++) {
		if (!ud->remths[i].use
		    && !ud->remths[i].bsy)
			break;
	}
	if (i >= MAXUSRDEVREMS)
		goto out;
	ud->remths[i].th = current_thread();
	ud->remths[i].use = 1;
	ud->remths[i].id = i;
	ret = i;
out:
	spinunlock(&ud->lock);
	printf("open %"PRIx64"! = %d", did, ret);
	return ret;
}

static void _usrdev_close(void *devopq, unsigned id)
{
	struct usrdev *ud = (struct usrdev *)devopq;
	/* Current thread: Process or Device */

	spinlock(&ud->lock);
	ud->remths[id].use = 0;
	spinunlock(&ud->lock);
	printf("close(%d)!\n", id);
}

static int _usrdev_io(void *devopq, unsigned id, uint64_t port, uint64_t val)
{
	/* Current thread: Process */
	struct usrdev *ud = (struct usrdev *)devopq;
	struct usrioreq *ior;

	ior = structs_alloc(&usrioreqs);
	ior->id = id;
	ior->port = port;
	ior->val = val;

	spinlock(&ud->lock);
	printf("ud->remths[%d].bsy = %d\n", id, ud->remths[id].bsy);
	if (ud->remths[id].bsy) {
		spinunlock(&ud->lock);
		structs_free(ior);
		return -1;
	}
	ud->remths[id].bsy = 1;
	TAILQ_INSERT_TAIL(&ud->ioreqs, ior, queue);
	spinunlock(&ud->lock);
	printf("io(%"PRIx64") = %"PRIx64"\n", port, val);
	thraise(ud->th, ud->sig);
	return 0;
}

static void _usrdev_intmap(void *devopq, unsigned id, unsigned intr, unsigned sig)
{
	struct usrdev *ud = (struct usrdev *)devopq;

	/* Current thread: Process */
	assert(id < MAXUSRDEVREMS);
	printf("intmap(%x) = %x\n", intr, sig);
}

static struct devops usrdev_ops = {
	.open = _usrdev_open,
	.close = _usrdev_close,
	.io = _usrdev_io,
	.intmap = _usrdev_intmap,
};

struct usrdev *usrdev_creat(uint64_t id, unsigned sig)
{
	struct usrdev *ud;

	ud = structs_alloc(&usrdevs);
	ud->lock = 0;
	ud->th = current_thread();
	ud->sig = sig;
	memset(&ud->remths, 0, sizeof(ud->remths));
	TAILQ_INIT(&ud->ioreqs);

	dev_init(&ud->dev, id, (void *)ud, &usrdev_ops);
	if (dev_attach(&ud->dev)) {
		structs_free(ud);
		ud = NULL;
	}
	return ud;
}

int usrdev_poll(struct usrdev *ud, uint64_t *p, uint64_t *v)
{
	int id;
	struct usrioreq *ior;

	spinlock(&ud->lock);
	ior = TAILQ_FIRST(&ud->ioreqs);
	TAILQ_REMOVE(&ud->ioreqs, ior, queue);
	spinunlock(&ud->lock);

	*p = ior->port;
	*v = ior->val;
	id = ior->id;
	structs_free(ior);
	printf("poll: returning %d\n", id);
	return id;
}

int usrdev_eio(struct usrdev *ud, unsigned id)
{
	if (id >= MAXUSRDEVREMS)
		return -1;
	spinlock(&ud->lock);
	printf("set busy of %d to zero\n", id);
	ud->remths[id].bsy = 0;
	spinunlock(&ud->lock);
	return 0;
}

void usrdev_destroy(struct usrdev *ud)
{
	dev_detach(&ud->dev);
	structs_free(ud);
}

void usrdevs_init(void)
{
	setup_structcache(&usrioreqs, usrioreq);
	setup_structcache(&usrdevs, usrdev);
}
