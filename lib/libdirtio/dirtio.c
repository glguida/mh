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


#include <sys/null.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/dirtio.h>
#include <sys/bitops.h>
#include <sys/atomic.h>
#include <inttypes.h>
#include <stdlib.h>
#include <syslib.h>

#include <microkernel.h>
#include <drex/preempt.h>
#include <drex/event.h>


int dirtio_desc_open(uint64_t nameid, void *map, struct dirtio_desc *dio)
{
	int id, ret, eioirq, kq;
	struct sys_creat_cfg cfg;
	struct dirtio_hdr *hdr = (struct dirtio_hdr *)map;

	id = sys_open(nameid);
	if (id < 0)
		return id;

	kq = drex_kqueue();
	if (kq < 0)
		return kq;

	eioirq = irqalloc();
	drex_kqueue_add(kq, EVFILT_DREX_IRQ, eioirq, 0);

	ret = sys_mapirq(id, 0, eioirq);
	if (ret) {
		sys_close(id);
		drex_kqueue_del(kq, EVFILT_DREX_IRQ, eioirq);
		irqfree(eioirq);
		return ret;
	}
	
	ret = sys_readcfg(id, &cfg);
	if (ret) {
		sys_close(id);
		drex_kqueue_del(kq, EVFILT_DREX_IRQ, eioirq);
		irqfree(eioirq);
		return ret;
	}

	if (cfg.vendorid != DIRTIO_VID) {
		sys_close(id);
		drex_kqueue_del(kq, EVFILT_DREX_IRQ, eioirq);
		irqfree(eioirq);
		return -EINVAL;
	}

	ret = sys_export(id, (u_long)map, 0);
	if (ret) {
		sys_close(id);
		drex_kqueue_del(kq, EVFILT_DREX_IRQ, eioirq);
		irqfree(eioirq);
		return ret;
	}

	dio->id = id;
	dio->hdr = hdr;

	dio->nameid = cfg.nameid;
	dio->vendorid = cfg.vendorid;
	dio->deviceid = cfg.deviceid;
	dio->eioirq = eioirq;
	dio->kq = kq;

	return 0;
}

int dirtio_mmio_inw(struct dirtio_desc *dio, uint32_t port,
		    uint8_t queue, uint64_t *val)
{
	int ret;
	unsigned fil;
	uintptr_t irq;
	uintptr_t udata;

	ret = sys_out(dio->id, PORT_DIRTIO_IN | (port << 8) | queue, 0);
	if (ret)
		return ret;
	drex_kqueue_wait(dio->kq, &fil, &irq, &udata, 0);
	*val = dio->hdr->ioval;
	return 0;
}

int dirtio_mmio_out(struct dirtio_desc *dio, uint32_t port,
		    uint8_t queue, uint64_t val)
{
	int ret;

	ret = sys_out(dio->id, ((port << 8) | queue)  & ~PORT_DIRTIO_IN, val);
	return ret;
}

int dirtio_mmio_outw(struct dirtio_desc *dio, uint32_t port,
		     uint8_t queue, uint64_t val)
{
	int ret;
	unsigned fil;
	uintptr_t irq;
	uintptr_t udata;

	ret = sys_out(dio->id, ((port << 8) | queue)  & ~PORT_DIRTIO_IN, val);
	if (ret)
		return ret;
	drex_kqueue_wait(dio->kq, &fil, &irq, &udata, 0);
	return 0;
}

void dirtio_desc_close(struct dirtio_desc *dio)
{

	sys_close(dio->id);
	irqfree(dio->eioirq);
}

#define GETBUF(_d, _i)							\
	((uint8_t *)(_d)->buf + (_d)->hdrmax + (_d)->bufsz * (_i))

void
dioqueue_init(struct dioqueue *dq, unsigned num, void *ioptr,
	      struct diodevice *dio)
{
	int i;

	dq->idx = 0;
	dq->dio = dio;
	dq->freedescs = malloc(sizeof(unsigned) * num);
	assert(dq->freedescs);
	dring_init(&dq->dring, 64, ioptr, 1);

	/* Mark all descriptors free */
	for (i = 0; i < num; i++)
		dq->freedescs[i] = i;
	dq->idx = i;
}

int
dioqueue_addv(struct dioqueue *dq, unsigned rdnum, unsigned wrnum,
	      struct iovec *iov)
{
	unsigned first = -1;
	unsigned i, idx;
	struct dring_desc *d, *prevd;
	struct dring_avail *da;
	
	/* dq->idx is also number of free elements */
	if (rdnum + wrnum >= dq->idx)
		return -ENOSPC;

	if (rdnum + wrnum == 0)
		return -EINVAL;

	prevd = NULL;
	for (i = 0; i < rdnum; i++, iov++) {
		/* Alloc descriptor */
		idx = dq->freedescs[--dq->idx];
		if (first == -1)
			first = idx;
		printf("Adding rd at desc %d\n", idx);
		d = &dq->dring.desc[idx];
		printf("d = %p\n", d);
		d->addr = (u_long)(iov->iov_base - dq->dio->buf);
		printf("addr = %lx\n", d->addr);
		d->len = iov->iov_len;
		d->flags |= DRING_DESC_F_NEXT;
		if (prevd != NULL)
			prevd->next = idx;
		prevd = d;
	}
	for (i = 0; i < wrnum; i++, iov++) {
		/* Alloc descriptor */
		idx = dq->freedescs[--dq->idx];
		if (first == -1)
			first = idx;
		printf("Adding wr at desc %d\n", idx);
		d = &dq->dring.desc[idx];
		d->addr = (u_long)(iov->iov_base - dq->dio->buf);
		d->len = iov->iov_len;
		d->flags |= DRING_DESC_F_WRITE | DRING_DESC_F_NEXT;
		if (prevd != NULL)
			prevd->next = idx;
		prevd = d;
	}
	d->flags &= ~DRING_DESC_F_NEXT;

	/* Update avail */
	da = dq->dring.avail;
	da->ring[da->idx % dq->dring.num] = first;
	membar_producer();
	da->idx++;
	printf("da idx is now %d (%p)\n", da->idx, dq->dring.desc);

	dirtio_mmio_outw(&dq->dio->desc, PORT_DIRTIO_NOTIFY, 0/*q*/, 0);
	return 0;
}

size_t
dirtio_allocv(struct diodevice *dio, struct iovec *iovec, int num,
		 size_t len)
{
	int i, n, b;
	struct iovec *iov = iovec;

	n = 0;
	for (i = 0; i < DIRTIO_BUFBMAPSZ(dio); i++) {
		printf("dio->bmap[i] = %llx\n", dio->bmap[i]);
		while (dio->bmap[i] != (uint64_t)-1) {
			b = ffs64(~dio->bmap[i]) - 1;
			assert(b != -1);
			dio->bmap[i] |= ((uint64_t)1 << b);
			iov->iov_base = GETBUF(dio, i * 64 + b);
			iov->iov_len = len < dio->bufsz ?
				len : dio->bufsz;
			len -= iov->iov_len;
			iov++;
			n++;
			if (len == 0 || n == num)
				goto out;
		}
	}
out:
	return len;
}

