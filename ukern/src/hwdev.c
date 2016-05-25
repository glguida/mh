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

#define MAXHWDEVIDS SYS_RDCFG_MAX_DEVIDS
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

struct memseg {
	paddr_t base;
	unsigned long len;
};

struct hwdev_cfg {
	uint64_t vid;
	uint64_t did[MAXHWDEVIDS];

	uint8_t nirqsegs;
	uint8_t npiosegs;

	uint8_t nmemsegs;
	struct memseg *memsegptr;

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
	return ret;
}

static int _hwdev_in(void *devopq, unsigned id, uint32_t port,
		      uint64_t * val)
{
	int i, found;
	struct hwdev *hd = (struct hwdev *) devopq;
	struct hwdev_cfg *cfg = hd->cfg;
	struct seg *ptr= cfg->segs + cfg->nirqsegs;
	uint8_t iosize = 1 << (port & IOPORT_SIZEMASK);
	uint16_t ioport = port >> IOPORT_SIZESHIFT;

	found = 0;
	for (i = 0; i < cfg->npiosegs; i++, ptr++) {
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
		printf("Unsupported I/O port size %d at port %llx\n", iosize, port);
		return -EINVAL;
	}
	return 0;
}

static int _hwdev_out(void *devopq, unsigned id, uint32_t port,
		       uint64_t val)
{
	int i, found;
	struct hwdev *hd = (struct hwdev *) devopq;
	struct hwdev_cfg *cfg = hd->cfg;
	struct seg *ptr= cfg->segs + cfg->nirqsegs;
	uint8_t iosize = 1 << (port & IOPORT_SIZEMASK);
	uint16_t ioport = port >> IOPORT_SIZESHIFT;

	found = 0;
	for (i = 0; i < cfg->npiosegs; i++, ptr++) {
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
			 paddr_t mmioaddr, pmap_prot_t prot)
{
	struct hwdev *hd = (struct hwdev *) devopq;
	pfn_t mmiopfn = (pfn_t)atop(mmioaddr);
	struct hwdev_cfg *cfg = hd->cfg;
	struct memseg *ptr= cfg->memsegptr;
	int i, found, ret;
	struct hwmap *hwmap;

	found = 0;
	for (i = 0; i < cfg->nmemsegs; i++, ptr++) {
		paddr_t start = ptr->base;
		paddr_t end = ptr->base + ptr->len;

		if (start <= mmioaddr && mmioaddr <= end) {
			found = 1;
			break;
		}
	}
	if (!found)
		return -EPERM;

	if ((va & PAGE_MASK) != (mmioaddr & PAGE_MASK)) {
		/* Do not confuse  the caller by thinking  one can get
		 * away with  mapping at  different page  offsets.  Do
		 * not  make  implicitly  a vfn-to-pfn  mapping  (thus
		 * ignoring the page  offset) as other implementations
		 * of other device types  might actually support this,
		 * e.g. through copying. */
		return -EINVAL;
	}

	if (!pfn_is_valid(mmiopfn))
		return -EINVAL;

	dprintf("mapping at addr %lx mmio %"PRIx64" (%d)\n",
	       va, (uint64_t)mmioaddr, prot);

	ret = iomap(va, mmiopfn, prot);
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


static int _hwdev_rdcfg(void *devopq, unsigned id, struct sys_rdcfg_cfg *cfg)
{
	int i;
	struct hwdev *hd = (struct hwdev *) devopq;
	struct hwdev_cfg *hdcfg = hd->cfg;

	cfg->vendorid = hdcfg->vid;
	for (i = 0; i < MAXHWDEVIDS; i++)
		cfg->deviceids[i] = hdcfg->did[i];

	cfg->niopfns = -1;
	cfg->nirqsegs = hdcfg->nirqsegs;
	cfg->npiosegs = hdcfg->npiosegs;
	cfg->nmemsegs = hdcfg->nmemsegs;

	for (i = 0; i < hdcfg->nirqsegs + cfg->npiosegs; i++) {
		cfg->segs[i].base = hdcfg->segs[i].base;
		cfg->segs[i].len = hdcfg->segs[i].len;
	}
	for (i = 0; i < hdcfg->nmemsegs; i++) {
		cfg->memsegs[i].base = hdcfg->memsegptr[i].base;
		cfg->memsegs[i].len = hdcfg->memsegptr[i].len;
	}
		
	return 0;
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
	for (i = 0; i < cfg->nirqsegs; i++, ptr++) {
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
	dprintf("hwdev: registering irq %d at thread %p with signal %d\n", irq, th,
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
	int i, segs, nmemsegs;
	size_t size;
	struct hwdev *hd;
	struct thread *th = current_thread();

	segs = syscfg->nirqsegs + syscfg->npiosegs;
	nmemsegs = syscfg->nmemsegs;

	size = sizeof(*hd);
	size += sizeof(struct hwdev_cfg);
	size += sizeof(struct seg) * segs;
	size += sizeof(struct memseg) * nmemsegs;

	hd = heap_alloc(size);
	hd->lock = 0;
	hd->th = th;
	memset(&hd->remths, 0, sizeof(hd->remths));
	hd->cfg = (struct hwdev_cfg *)(hd+1);

	hd->cfg->vid = syscfg->vendorid;
	for (i = 0; i < MAXHWDEVIDS; i++)
		hd->cfg->did[i] = syscfg->deviceids[i];
	hd->cfg->nirqsegs = syscfg->nirqsegs;
	hd->cfg->npiosegs = syscfg->npiosegs;
	hd->cfg->nmemsegs = syscfg->nmemsegs;

	for (i = 0; i < segs; i++) {
		hd->cfg->segs[i].base = syscfg->segs[i].base;
		hd->cfg->segs[i].len = syscfg->segs[i].len;
	}
	hd->cfg->memsegptr = (struct memseg *)&hd->cfg->segs[i];
	for (i = 0; i < nmemsegs; i++) {
		hd->cfg->memsegptr[i].base = syscfg->memsegs[i].base;
		hd->cfg->memsegptr[i].len = syscfg->memsegs[i].len;
	}

	dev_init(&hd->dev, syscfg->nameid, (void *) hd, &hwdev_ops, th->euid, th->egid, mode);
	if (dev_attach(&hd->dev)) {
		heap_free(hd);
		hd = NULL;
	}
	return hd;
}
