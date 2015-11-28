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
#include <uk/locks.h>
#include <uk/structs.h>
#include <uk/rbtree.h>
#include <uk/kern.h>
#include <machine/uk/cpu.h>
#include <lib/lib.h>
#include <uk/device.h>

static struct slab devdescs;
static struct slab devices;

static lock_t device_rbtree_lock = 0;
static rb_tree_t device_rbtree;

/* Process Side */
int device_io(struct devdesc *dd, uint64_t port, uint64_t val)
{
	struct device *d;
	
	spinlock(&dd->lock);
	if (dd->devstatus & DEVSTATUS_IOPEND) {
		spinunlock(&dd->lock);
		return -1;
	}
	dd->io_port = port;
	dd->io_val = val;
	dd->devstatus |= DEVSTATUS_IOPEND;
	d = dd->dev;
	spinunlock(&dd->lock);

	thraise(d->th, d->signo);
	return 0;
}
 
/* Process Side */
int device_intmap(struct devdesc *dd, unsigned id, unsigned sig)
{
	if (id >= MAXDEVINTRS || sig > MAXSIGNALS)
		return -1;

	spinlock(&dd->lock);
	dd->intmap[id] = sig;
	spinunlock(&dd->lock);
	return 0;
}

/* Process Side */
struct devdesc *device_open(uint64_t id)
{
	int i;
	struct device *d;
	struct devdesc *dd;

	spinlock(&device_rbtree_lock);
	d = rb_tree_find_node(&device_rbtree, (void *)&id);
	spinunlock(&device_rbtree_lock);

	if (!d)
		return NULL;

	dd = structs_alloc(&devdescs);
	dd->lock = 0;
	dd->dev = d;
	dd->th = current_thread();
	dd->devctl = DEVCTL_ENABLED;
	dd->devstatus = 0;

	for (i = 0; i < MAXDEVINTRS; i++)
		dd->intmap[i] = -1;

	spinlock(&d->lock);
	LIST_INSERT_HEAD(&d->descs, dd, list);
	spinunlock(&d->lock);
	return dd;
}

/* Device Side */
struct device *device_creat(uint64_t id, unsigned signo)
{
	struct thread *th = current_thread();
	struct device *d;

	if (signo >= MAXSIGNALS)
		return NULL;

	d = structs_alloc(&devices);
	d->lock = 0;
	d->id = id;
	d->th = th;
	d->signo = signo;
	LIST_INIT(&d->descs);	

	/* Register */
	if (rb_tree_find_node(&device_rbtree, &id)) {
		spinunlock(&device_rbtree_lock);
		printf("device %"PRIx64" already existing\n", id);
		structs_free(d);		
		return NULL;
	}
	rb_tree_insert_node(&device_rbtree, (void *)d);
	spinunlock(&device_rbtree_lock); 
	return d;
}

void device_free(struct device *d)
{
	spinlock(&device_rbtree_lock);
	rb_tree_remove_node(&device_rbtree, (void *)d);
	spinunlock(&device_rbtree_lock);

	memset(d, 0, sizeof(struct device));
	structs_free(d);
}

static int devtree_compare_key(void *ctx, const void *n, const void *key)
{
	const struct device *dev = n;
	const uint64_t cid = *(const uint32_t *) key;
	const uint64_t id = dev->id;

	if (cid < id)
		return 1;
	if (cid > id)
		return -1;
	return 0;
}

static int devtree_compare_nodes(void *ctx, const void *n1, const void *n2)
{
	const struct device *d1 = n1;
	const struct device *d2 = n2;
	const uint64_t id1 = d1->id;
	const uint64_t id2 = d2->id;

	if (id1 < id2)
		return -1;
	if (id1 > id2)
		return 1;
	return 0;
}

static const rb_tree_ops_t device_tree_ops = {
	.rbto_compare_nodes = devtree_compare_nodes,
	.rbto_compare_key = devtree_compare_key,
	.rbto_node_offset = offsetof(struct device, rb_node),
};

void devices_init(void)
{
	/* Bring up setup infrastructure */
	setup_structcache(&devdescs, devdesc);
	setup_structcache(&devices, device);
	rb_tree_init(&device_rbtree, &device_tree_ops);
	device_rbtree_lock = 0;
}
