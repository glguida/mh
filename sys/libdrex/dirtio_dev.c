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


#include <sys/types.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <machine/vmparam.h>
#include <microkernel.h>
#include <sys/dirtio.h>
#include <assert.h>
#include <drex/lwt.h>
#include <drex/softirq.h>
#include "vmap.h"

#define MEMCACHESIZE 16L

static struct memcache {
	unsigned id;
	ioaddr_t ioaddr;
	void *vaddr;
	unsigned ref;
	TAILQ_ENTRY(memcache) list;
} memcache[MEMCACHESIZE];

static TAILQ_HEAD(mchead ,memcache) memcache_lruq =
	TAILQ_HEAD_INITIALIZER(memcache_lruq);

void *dirtio_mem_get(unsigned id, ioaddr_t ioaddr)
{
	int ret;
	struct memcache *mc;

	/* Search by the LRU */
	TAILQ_FOREACH_REVERSE(mc, &memcache_lruq, mchead, list) {
		if (mc->id == id && mc->ioaddr == ioaddr) {
			mc->ref++;
			return mc->vaddr;
		}
	}

	mc = TAILQ_FIRST(&memcache_lruq);
	TAILQ_REMOVE(&memcache_lruq, mc, list);
	assert(mc->ref == 0);

	ret = sys_import(id, ioaddr, (vaddr_t)mc->vaddr);
	if (ret != 0) {
		TAILQ_INSERT_HEAD(&memcache_lruq, mc, list);
		return NULL;
	}
	TAILQ_INSERT_TAIL(&memcache_lruq, mc, list);

	mc->ref++;
	mc->id = id;
	mc->ioaddr = ioaddr;
	return mc->vaddr;
}

void dirtio_mem_put(void *addr)
{
	struct memcache *mc;

	/* XXX: faster way: use addr - start to get memcache entry */
	TAILQ_FOREACH_REVERSE(mc, &memcache_lruq, mchead, list)
		if (mc->vaddr == addr) {
			assert(mc->ref != 0);
			mc->ref--;
			return;
		}
	assert(0);
}

void dirtio_mem_init(void)
{
	int i;
	vaddr_t start = vmap_alloc(MEMCACHESIZE * PAGE_SIZE, VFNT_IMPORT);

	printf("Alloc from %lx\n", start);
	for (i = 0; i < MEMCACHESIZE; i++) {
		memcache[i].id = -1;
		memcache[i].ioaddr = 0;
		memcache[i].vaddr = (void *)(start + (i * PAGE_SIZE));
		memcache[i].ref = 0;
		TAILQ_INSERT_HEAD(&memcache_lruq, memcache + i, list);
	}
}

struct dirtio_dev __dev;

int dirtio_dev_return(unsigned id, uint32_t val)
{
	struct dirtio_hdr *hdr;

	/* XXX: Do a usercpy-like function. We might fault if remote
	 * doesn't export memory! */
	hdr = (struct dirtio_hdr *)dirtio_mem_get(id, 0);
	if (hdr == NULL)
		return -ENOENT;
	hdr->ioval = val;
	dirtio_mem_put(hdr);
	return 0;
}

int dirtio_dev_io(unsigned id, uint64_t port, uint64_t val)
{
	int ret = -EINVAL;
	int read = 0;
	unsigned field, queue;

	if (port & PORT_DIRTIO_IN)
		read = 1;

	port &= ~PORT_DIRTIO_IN;
	field = port >> 8;
	queue = port & 0xff;
	if (read) {
		/* Read */
		switch (field) {
		case PORT_DIRTIO_MAGIC:
			val = DIRTIO_MAGIC;
			break;
		case PORT_DIRTIO_VER:
			val = DIRTIO_VER;
			break;
		case PORT_DIRTIO_QMAX:
			if (queue >= __dev.queues)
				goto read_err;
			val = __dev.qmax[queue];
			break;
		case PORT_DIRTIO_QSIZE:
			if (queue >= __dev.queues)
				goto read_err;
			val = __dev.qsize[queue];
			break;
		case PORT_DIRTIO_READY:
			if (queue >= __dev.queues)
				goto read_err;
			val = __dev.qready[queue];
			break;
		case PORT_DIRTIO_ISR:
			val = __dev.isr;
			break;
		case PORT_DIRTIO_DSR:
			val = __dev.dsr;
			break;
		read_err:
		default:
			val = -1;
			break;
		}
		ret = dirtio_dev_return(id, val);
	} else {
		/* Write */
		field = (port) >> 8;
		queue = (port) & 0xff;

		switch (field) {
		case PORT_DIRTIO_QSIZE:
			if (queue < __dev.queues && val < __dev.qmax[queue]) {
				__dev.qsize[queue] = val;
				ret = 0;
			}
			break;
		case PORT_DIRTIO_NOTIFY:
			if (queue < __dev.queues)
				ret = __dev.notify(queue);
			break;
		default:
			break;
		}
	}
	return ret;
    
}

#if 0
int
dirtio_dev_wait(struct dirtio_dev *dev, struct sys_poll_ior *ior)
{
	int id;

	preempt_disable();
	id = sys_poll(&ior);
	while (id < 0) {
		sys_wait();
		preempt_restore();
		id = sys_poll(ior);
	}
	preempt_enable();

}
#endif

void
dirtio_dev_init(unsigned queues,
		unsigned *qmax, unsigned *qsize, unsigned *qready)
{
	dirtio_mem_init();
	__dev.queues = queues;
	__dev.qmax = qmax;
	__dev.qsize = qsize;
	__dev.qready = qready;
	__dev.isr = 0;
	__dev.dsr = 0;
}

static void
__dirtio_dev_process(void *arg)
{
	unsigned id;
	struct sys_poll_ior ior;

	printf("Process!\n");
	id = sys_poll(&ior);
	dirtio_dev_io(id, ior.port, ior.val);
	sys_eio(id);
	lwt_exit();
}

int
dirtio_dev_creat(struct sys_creat_cfg *cfg, devmode_t mode)
{
	int ret;
	unsigned irq = irqalloc();

	ret = sys_creat(cfg, irq, mode);
	if (ret != 0) {
		irqfree(irq);
		return ret;
	}
	softirq_register(irq, __dirtio_dev_process, NULL);
	return 0;
}
