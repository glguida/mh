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
#include <drex/lwt.h>
#include <drex/softirq.h>
#include <drex/vmap.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <syslib.h>
#include <sys/atomic.h>
#include <sys/dirtio.h>

struct dirtio_dev __dev;

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
	int pgoff = ioaddr & PAGE_MASK;
	struct memcache *mc;

	ioaddr = trunc_page(ioaddr);
	/* Search by the LRU */
	TAILQ_FOREACH_REVERSE(mc, &memcache_lruq, mchead, list) {
		if (mc->id == id && mc->ioaddr == ioaddr) {
			mc->ref++;
			return mc->vaddr + pgoff;
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
	return mc->vaddr + pgoff;
}

void dirtio_mem_put(void *addr)
{
	struct memcache *mc;

	addr = (void *)trunc_page((uintptr_t)addr);

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

int
dioqueue_dev_getv(unsigned id, unsigned queue,
		  struct dirtio_dev_iovec **diovecp,
		  unsigned *num)
{
	int len = 0;
  	struct dirtio_dev_iovec *diov = NULL;
	int i;
	uint16_t lidx, idx;
	struct iovec *iov;
	struct dring_dev *dr;
	struct dring_desc *d;
	struct dring_avail *da;

	if (queue >= __dev.queues)
		return -EINVAL;

	dr = &__dev.dioqueues[queue].dring;
	da = dirtio_mem_get(id, dr->avail);
	if (da == NULL)
		return -EFAULT;
	printf("dr->desc = %lx\n", dr->desc);
	d = dirtio_mem_get(id, dr->desc);
	if (d == NULL) {
		dirtio_mem_put(da);
		return -EFAULT;
	}

	lwt_exception({
       		if (diov != NULL) {
			for (i = 0; i < len; i++)
				if (diov->iov[i].iov_base != NULL)
					dirtio_mem_put(diov->iov[i].iov_base);
			free(diov);
		}
		dirtio_mem_put(da);
		dirtio_mem_put(d);
		return -EIO;
	});

	lidx = __dev.dioqueues[queue].last_seen_avail;
	idx = da->idx;
	membar_consumer();

	if (lidx == idx) {
		dirtio_mem_put(da);
		dirtio_mem_put(d);
		lwt_exception_clear();
		return 0; /* No messages */
	}

	idx = da->ring[lidx % dr->num];
	for (i = 0; i < dr->num; i++) {
		if (idx >= dr->num) {
			printf("IDX IS %d\n", idx);
			dirtio_mem_put(da);
			dirtio_mem_put(d);
			lwt_exception_clear();			
			return -EFAULT;
		}
		if (!(d[idx].flags & DRING_DESC_F_NEXT))
			break;
		idx = d[idx].next;
	}
	/* Check invalid length (or loop) */
	if (i == dr->num) {
		dirtio_mem_put(da);
		dirtio_mem_put(d);
		lwt_exception_clear();
		return -EFAULT;
	}

	len = i + 1;
	diov = malloc(sizeof(*diov) + sizeof(struct iovec) * len);
	iov = diov->iov;
	memset(diov->iov, 0, sizeof(struct iovec) * len);
	/* Re-check basics. Driver might have changed things. */
	idx = da->ring[lidx % dr->num];
	diov->idx = idx;
	for (i = 0; i < dr->num; i++) {
		if (idx >= dr->num) {
			printf("Wrong descidx (%d) in ring (2) [%d]\n",
			       idx, id);
			goto out_bad;
		}

		iov[i].iov_base = dirtio_mem_get(id, d[idx].addr);
		if(iov[i].iov_base == NULL) {
			printf("Cannot map base %lx\n", d[idx].addr);
			goto out_bad;
		}
		iov[i].iov_len = d[idx].len;

		if (!(d[idx].flags & DRING_DESC_F_NEXT))
			break;
		/* If we're here after len passes, then the driver has
		   corrupted the structure while we scan it. Bad, bad
		   driver. */
		if (i == len - 1) {
			printf("Reached end of iov, desc changed! [%d]\n", id);
			goto out_bad;
		}
		idx = d[idx].next;
	}
	dirtio_mem_put(da);
	dirtio_mem_put(d);
	*diovecp = diov;
	*num = len;
	lwt_exception_clear();
	return 1;

out_bad:
	dirtio_mem_put(da);
	dirtio_mem_put(d);
	for (i = 0; i < len; i++)
		if (iov[i].iov_base != NULL)
			dirtio_mem_put(iov[i].iov_base);
	free(diov);
	lwt_exception_clear();
	return -EFAULT;;
}

int dirtio_dev_putv(unsigned id, unsigned queue, struct iovec *iovec,
		       unsigned num)
{
	
}

int dirtio_dev_return(unsigned id, uint32_t val)
{
	struct dirtio_hdr *hdr;

	hdr = (struct dirtio_hdr *)dirtio_mem_get(id, 0);
	if (hdr == NULL)
		return -ENOENT;

	lwt_exception({
			dirtio_mem_put(hdr);
			return -EIO;
		});
	hdr->ioval = val;
	lwt_exception_clear();
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
				ret = __dev.notify(id, queue);
			break;
		default:
			/* Writing to DSR: DEVICE RESET */
			break;
		}
	}
	return ret;
    
}

void
dirtio_dev_init(unsigned queues, int (*notify)(unsigned, unsigned),
		unsigned *qmax, unsigned *qsize, unsigned *qready,
		struct dioqueue_dev *dqs)
{
	dirtio_mem_init();
	__dev.queues = queues;
	__dev.qmax = qmax;
	__dev.qsize = qsize;
	__dev.qready = qready;
	__dev.dsr = 0;
	__dev.dioqueues = dqs;
	__dev.notify = notify;
}

void
__dirtio_dev_process(void)
{
	unsigned id;
	struct sys_poll_ior ior;

	printf("Process! Wohoo!\n");
	id = sys_poll(&ior);
	dirtio_dev_io(id, ior.port, ior.val);
	sys_eio(id);
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
	irq_set_dirtio(irq);
	return 0;
}
