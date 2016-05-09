/*
 * Copyright (c) 2016, Gianluca Guida
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
#include <uk/pfndb.h>
#include <uk/hwdev.h>
#include <machine/uk/cpu.h>
#include <machine/uk/platform.h>
#include <uk/pgalloc.h>
#include <uk/heap.h>
#include <uk/errno.h>
#include <lib/lib.h>

#define MAXHWDEVREMS 256

struct hwsig {
	struct irqsig irqsig;
	LIST_ENTRY(hwsig) list;
};

struct hwmap {
	vaddr_t va;
	LIST_ENTRY(hwmap) list;
};

struct remth {
	int use:1;			/* Entry in use */
	LIST_HEAD(, hwsig) hwsigs; 	/* Signals mapped */
	LIST_HEAD(, hwmap) hwmaps;	/* MMIO mapped */
};

struct seg {
	uint16_t base;
	uint16_t len;
};

struct hwdev_cfg {
	uint32_t vid;
	uint32_t did;

	uint8_t irqsegs;
	uint8_t piosegs;
	uint8_t memsegs;
	struct seg segs[0];
};

struct hwdev {
	lock_t lock;
	struct dev dev;		/* Registered device */
	struct thread *th;	/* Device thread */

	struct remth remths[MAXHWDEVREMS];

	struct hwdev_cfg *cfg;	/* Device Configuration Space */
};

static int _hwdev_open(void *devopq, uint64_t did)
{
	int i, ret;
	struct hwdev *hd = (struct hwdev *) devopq;

	spinlock(&hd->lock);
	for (i = 0; i < MAXHWDEVREMS; i++) {
		if (!hd->remths[i].use);
			break;
	}
	if (i >= MAXHWDEVREMS) {
		ret = -ENFILE;
		goto out;
	}
	hd->remths[i].use = 1;
	LIST_INIT(&hd->remths[i].hwsigs);
	LIST_INIT(&hd->remths[i].hwmaps);
	ret = i;
      out:
	spinunlock(&hd->lock);
	printf("open %" PRIx64 "! = %d", did, ret);
	return ret;
}

static int _hwdev_in(void *devopq, unsigned id, uint64_t port,
		      uint64_t * val)
{
	int i, found;
	struct hwdev *hd = (struct hwdev *) devopq;
	struct hwdev_cfg *cfg = hd->cfg;
	struct seg *ptr= cfg->segs + cfg->irqsegs;
	uint8_t iosize = port & PLTPORT_SIZEMASK;
	uint16_t ioport = port >> PLTPORT_SIZESHIFT;

	found = 0;
	for (i = 0; i < cfg->piosegs; i++, ptr++) {
		uint16_t start = ptr->base;
		uint16_t end = ptr->base + ptr->len - 1;

		if (start <= ioport && ioport <= end) {
			found = 1;
			break;
		}
	}
	if (!found)
		return -EPERM;


	switch (iosize) {
	case 1:
		*val = platform_inb(ioport);
		break;
	case 2:
		*val = platform_inw(ioport);
		break;
	case 4:
		*val = platform_inl(ioport);
		break;
	default:
		printf("Meh: %llx\n", port);
		return -EINVAL;
	}
	return 0;
}

static int _hwdev_out(void *devopq, unsigned id, uint64_t port,
		       uint64_t val)
{
	int i, found;
	struct hwdev *hd = (struct hwdev *) devopq;
	struct hwdev_cfg *cfg = hd->cfg;
	struct seg *ptr= cfg->segs + cfg->irqsegs;
	uint8_t iosize = port & PLTPORT_SIZEMASK;
	uint16_t ioport = port >> PLTPORT_SIZESHIFT;

	found = 0;
	for (i = 0; i < cfg->piosegs; i++, ptr++) {
		uint16_t start = ptr->base;
		uint16_t end = ptr->base + ptr->len - 1;

		if (start <= ioport && ioport <= end) {
			found = 1;
			break;
		}
	}
	if (!found)
		return -EPERM;


	switch (iosize) {
	case 1:
		platform_outb(ioport, val);
		break;
	case 2:
		platform_outw(ioport, val);
		break;
	case 4:
		platform_outl(ioport, val);
		break;
	default:
		printf("Meh: %llx\n", port);
		return -EINVAL;
	}
	return 0;
}

static int _hwdev_export(void *devopq, unsigned id, vaddr_t va,
			  unsigned iopfn)
{
	return -ENODEV;
}

static int _hwdev_iomap(void *devopq, unsigned id, vaddr_t va,
			 pfn_t mmiopfn, pmap_prot_t prot)
{
	int i, found, ret;
	struct hwmap *hwmap;
	struct hwdev *hd = (struct hwdev *) devopq;
	struct hwdev_cfg *cfg = hd->cfg;
	struct seg *ptr= cfg->segs + cfg->irqsegs + cfg->piosegs;

	found = 0;
	for (i = 0; i < cfg->memsegs; i++, ptr++) {
		uint16_t start = ptr->base;
		uint16_t end = ptr->base + ptr->len -1;

		if (start <= mmiopfn && mmiopfn <= end) {
			found = 1;
			break;
		}
	}
	if (!found)
		return -EPERM;

	printf("mapping at addr %lx pfn %lx (%d)\n", va, mmiopfn, prot);
	if (!pfn_is_valid(mmiopfn))
		return -EINVAL;

	ret = _iomap(va, mmiopfn, prot);
	if (ret < 0)
		return ret;

	hwmap = heap_alloc(sizeof(*hwmap));
	hwmap->va = va;
	spinlock(&hd->lock);
	LIST_INSERT_HEAD(&hd->remths[id].hwmaps, hwmap, list);
	spinunlock(&hd->lock);
	return 0;
}

static int _hwdev_iounmap(void *devopq, unsigned id, vaddr_t va)
{
	struct hwdev *hd = (struct hwdev *) devopq;
	struct hwmap *pm, *tpm;

	spinlock(&hd->lock);
	assert(hd->remths[id].use);
	LIST_FOREACH_SAFE(pm, &hd->remths[id].hwmaps, list, tpm) {
		if (pm->va == va) {
			LIST_REMOVE(pm, list);
			vmunmap(va);
			heap_free(pm);
			spinunlock(&hd->lock);
			return 0;
		}
	}
	spinunlock(&hd->lock);
	return -ENOENT;
}


static int _hwdev_rdcfg(void *devopq, unsigned id, struct sys_creat_cfg *cfg)
{
	/* XXX: pass buffer and size, and fill accordingly. */
	return -ENODEV;
}

static int _hwdev_irqmap(void *devopq, unsigned id, unsigned irq,
			  unsigned sig)
{
	struct hwdev *hd = (struct hwdev *) devopq;
	struct hwdev_cfg *cfg = hd->cfg;
	struct thread *th = current_thread();
	struct seg *ptr = hd->cfg->segs;
	struct hwsig *hwsig;
	int i, found, ret;

	found = 0;
	for (i = 0; i < cfg->irqsegs; i++, ptr++) {
		uint16_t start = ptr->base;
		uint16_t end = ptr->base + ptr->len - 1;

		if (start <= irq && irq <= end) {
			found = 1;
			break;
		}
	}
	if (!found)
		return -EPERM;
	
	hwsig = heap_alloc(sizeof(struct hwsig));
	printf("Registering irq %d at thread %p with signal %d\n", irq, th,
	       sig);
	ret = irqregister(&hwsig->irqsig, irq, th, sig,
			  0 /* No filter */ );
	if (ret < 0) {
		heap_free(hwsig);
		return ret;
	}

	spinlock(&hd->lock);
	LIST_INSERT_HEAD(&hd->remths[id].hwsigs, hwsig, list);
	spinunlock(&hd->lock);
	return ret;
}

static void _hwdev_close(void *devopq, unsigned id)
{
	struct hwdev *hd = (struct hwdev *) devopq;
	struct hwsig *ps, *tps;
	struct hwmap *pm, *tpm;

	spinlock(&hd->lock);
	assert(hd->remths[id].use);
	LIST_FOREACH_SAFE(pm, &hd->remths[id].hwmaps, list, tpm) {
		LIST_REMOVE(pm, list);
		vmunmap(pm->va);
		heap_free(pm);
	}
	LIST_FOREACH_SAFE(ps, &hd->remths[id].hwsigs, list, tps) {
		LIST_REMOVE(ps, list);
		irqunregister(&ps->irqsig);
		heap_free(ps);
	}
	hd->remths[id].use = 0;
	spinunlock(&hd->lock);
}

static struct devops hwdev_ops = {
	.open = _hwdev_open,
	.close = _hwdev_close,
	.in = _hwdev_in,
	.out = _hwdev_out,
	.export = _hwdev_export,
	.rdcfg = _hwdev_rdcfg,
	.irqmap = _hwdev_irqmap,
};

struct hwdev *hwdev_creat(struct sys_hwcreat_cfg *syscfg, devmode_t mode)
{
	int i, segs;
	size_t size;
	struct hwdev *hd;
	struct thread *th = current_thread();

	segs = syscfg->irqsegs + syscfg->piosegs + syscfg->memsegs;

	size = sizeof(*hd);
	size += sizeof(struct hwdev_cfg);
	size += sizeof(struct seg) * segs;

	hd = heap_alloc(size);
	hd->lock = 0;
	hd->th = th;
	memset(&hd->remths, 0, sizeof(hd->remths));
	hd->cfg = (struct hwdev_cfg *)(hd+1);

	hd->cfg->vid = syscfg->vendorid;
	hd->cfg->did = syscfg->deviceid;
	hd->cfg->irqsegs = syscfg->irqsegs;
	hd->cfg->piosegs = syscfg->piosegs;
	hd->cfg->memsegs = syscfg->memsegs;

	printf("Registering %llx : %d %d %d\n", syscfg->nameid, hd->cfg->irqsegs, hd->cfg->piosegs, hd->cfg->memsegs);

	for (i = 0; i < segs; i++) {
		hd->cfg->segs[i].base = syscfg->segs[i].base;
		hd->cfg->segs[i].len = syscfg->segs[i].len;
	}
	dev_init(&hd->dev, syscfg->nameid, (void *) hd, &hwdev_ops, th->euid, th->egid, mode);
	if (dev_attach(&hd->dev)) {
		heap_free(hd);
		hd = NULL;
	}
	return hd;
}
