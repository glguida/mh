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
#include <uk/rbtree.h>
#include <uk/bus.h>
#include <uk/structs.h>
#include <uk/assert.h>
#include <uk/kern.h>
#include <machine/uk/cpu.h>
#include <lib/lib.h>

static lock_t sys_device_rbtree_lock = 0;
static rb_tree_t sys_device_rbtree;

/* Bus plug:
 *
 * . Retrieve device from SDL.
 * . bus lock
 * . dev lock
 * . if offline GTFO
 * . b->devs[i].opq = d->open();
 * . dev->buses add b
 * . dev unlock
 * . b->devs[i].dev = dev
 * . bus unlock
 */

int bus_plug(struct bus *b, uint64_t did)
{
	int i, devid, ret = -1;
	struct dev *d;

	spinlock(&sys_device_rbtree_lock);
	d = rb_tree_find_node(&sys_device_rbtree, (void *)&did);
	spinunlock(&sys_device_rbtree_lock);

	if (d == NULL)
		return -1;

	spinlock(&b->lock);

	for (i = 0; i < MAXBUSDEVS; i++) {
		if (!b->devs[i].bsy
		    && !b->devs[i].dev)
			break;
	}
	if (i >= MAXBUSDEVS)
		goto out;

	/* Plug device */
	spinlock(&d->lock);
	if (d->offline) {
		spinunlock(&d->lock);
		goto out;
	}
	devid = d->ops->open(d->devopq, did);
	if (devid < 0) {
		spinunlock(&d->lock);
		goto out;
	}
	LIST_INSERT_HEAD(&d->busdevs, b->devs + i, list);
	b->devs[i].plg = 1;
	spinunlock(&d->lock);
	b->devs[i].bus = b;
	b->devs[i].dev = d;
	b->devs[i].devid = devid;

	/* Mark it open. */
	b->devs[i].bsy = 1;
	ret = i;
out:
	spinunlock(&b->lock);
	return ret;
}

/* Bus unplug:
 *
 * . bus lock
 * . bus->devs[i].dev lock
 * . if offline GTFO
 * . d->close(bus->devs[i].opq);
 * . dev->bues del b
 * . bus->devs[i].dev unlock
 * . bus->devs[i].opq
 * . bus unlock
 */

int bus_unplug(struct bus *b, unsigned desc)
{
	int ret = -1;
	struct dev *d;

	if (desc >= MAXBUSDEVS)
		return -1;

	spinlock(&b->lock);
	d = b->devs[desc].dev;
	if (!b->devs[desc].plg)
		goto no_unplug;
	assert(d != NULL);

	/* Unplug device */
	spinlock(&d->lock);
	if (d->offline) {
		spinunlock(&d->lock);
		goto no_unplug;
	}
	d->ops->close(d->devopq, b->devs[desc].devid);
	LIST_REMOVE(b->devs + desc, list);
	b->devs[desc].plg = 0;
	spinunlock(&d->lock);

	b->devs[desc].bus = NULL;
	b->devs[desc].dev = NULL;
	b->devs[desc].devid = 0;
	ret = 0;
no_unplug:
	b->devs[desc].bsy = 0;
	spinunlock(&b->lock);
	return ret;
}

int bus_io(struct bus *b, unsigned desc, uint64_t port, uint64_t val)
{
	int ret = -1;
	struct dev *d;

	if (desc >= MAXBUSDEVS)
		return -1;

	spinlock(&b->lock);
	d = b->devs[desc].dev;
	if (!b->devs[desc].plg)
		goto out_io;
	assert(d != NULL);

	spinlock(&d->lock);
	if (d->offline) {
		spinunlock(&d->lock);
		goto out_io;
	}
	ret = d->ops->io(d->devopq, b->devs[desc].devid, port, val);
	spinunlock(&d->lock);
out_io:
	spinunlock(&b->lock);
	return ret;
}

int bus_export(struct bus *b, unsigned desc, vaddr_t va, unsigned iopfn)
{
	int ret = -1;
	struct dev *d;

	if (desc >= MAXBUSDEVS)
		return -1;

	spinunlock(&b->lock);
	d = b->devs[desc].dev;
	if (!b->devs[desc].plg) {
		printf("Uh? %d\n", desc);
		goto out_io;
	}
	assert(d != NULL);
	spinlock(&d->lock);
	if (d->offline) {
		spinunlock(&d->lock);
		goto out_io;
	}
	ret = d->ops->export(d->devopq, b->devs[desc].devid, va, iopfn);
	spinunlock(&d->lock);
      out_io:
	return ret;
}

int bus_irqmap(struct bus *b, unsigned desc, unsigned irq, unsigned sig)
{
	int ret = -1;
	struct dev *d;

	printf("Buh!\n");
	if (desc >= MAXBUSDEVS)
		return -1;

	spinunlock(&b->lock);
	d = b->devs[desc].dev;
	if (!b->devs[desc].plg) {
		printf("Uh? %d\n", desc);
		goto out_io;
	}
	assert(d != NULL);

	spinlock(&d->lock);
	if (d->offline) {
		spinunlock(&d->lock);
		goto out_io;
	}
	ret = d->ops->irqmap(d->devopq, b->devs[desc].devid, irq, sig);
	spinunlock(&d->lock);
out_io:
	return ret;
}

int dev_attach(struct dev *d)
{
	spinlock(&sys_device_rbtree_lock);
	if (rb_tree_find_node(&sys_device_rbtree, &d->did)) {
		spinunlock(&sys_device_rbtree_lock);
		printf("device %"PRIx64" already existing\n", d->did);
		return -1;
	}
	rb_tree_insert_node(&sys_device_rbtree, (void *)d);
	spinunlock(&sys_device_rbtree_lock);

	return 0;
}

/* Device detach:
 *
 * . Deregister from SDL
 *
 * . device lock
 * . Set device as offline. Do not attempt any further operation
 *   (open, io, irqmap, close)
 * . Walk the list and put in a destroy list.
 * . device unlock
 *
 * .foreach bus
 * .    take the bus lock
 * .    take the device lock
 * .    close(opq)
 * .    off device lock
 * .    bus->dev = NULL
 * .    bus->opq = NULL
 * .    
 * .    off bus lock
 */

void dev_detach(struct dev *d)
{
	struct bdeve *bd, *td;
	LIST_HEAD(,bdeve) destroy_list;

	spinlock(&sys_device_rbtree_lock);
	rb_tree_remove_node(&sys_device_rbtree, (void *)d);
	spinunlock(&sys_device_rbtree_lock);

	LIST_INIT(&destroy_list);
	spinlock(&d->lock);
	d->offline = 1;
	LIST_FOREACH_SAFE(bd, &d->busdevs, list, td) {
		LIST_REMOVE(bd, list);
		LIST_INSERT_HEAD(&destroy_list, bd, list);
	}
	spinunlock(&d->lock);

	LIST_FOREACH(bd, &destroy_list, list) {
		struct bus *b = bd->bus;

		spinlock(&b->lock);
		spinlock(&d->lock);
		d->ops->close(d->devopq, bd->devid);
		bd->plg = 0;
		spinunlock(&d->lock);
		bd->bus = NULL;
		bd->dev = NULL;
		bd->devid = 0;
		spinunlock(&b->lock);
	}
}

void dev_init(struct dev *d, uint64_t id, void *opq, struct devops *ops)
{
	d->did = id;
	d->lock = 0;
	d->offline = 0;
	LIST_INIT(&d->busdevs);
	d->devopq = opq;
	d->ops = ops;
}

void dev_free(struct dev *d)
{

	structs_free(d);
}

static int devtree_compare_key(void *ctx, const void *n, const void *key)
{
	const struct dev *dev = n;
	const uint64_t cid = *(const uint64_t *) key;
	const uint64_t id = dev->did;

	if (cid < id)
		return 1;
	if (cid > id)
		return -1;
	return 0;
}

static int devtree_compare_nodes(void *ctx, const void *n1, const void *n2)
{
	const struct dev *d1 = n1;
	const struct dev *d2 = n2;
	const uint64_t id1 = d1->did;
	const uint64_t id2 = d2->did;

	if (id1 < id2)
		return -1;
	if (id1 > id2)
		return 1;
	return 0;
}

static const rb_tree_ops_t sys_device_tree_ops = {
	.rbto_compare_nodes = devtree_compare_nodes,
	.rbto_compare_key = devtree_compare_key,
	.rbto_node_offset = offsetof(struct dev, rb_node),
};

void devices_init(void)
{
	rb_tree_init(&sys_device_rbtree, &sys_device_tree_ops);
	sys_device_rbtree_lock = 0;

	usrdevs_init();
}
