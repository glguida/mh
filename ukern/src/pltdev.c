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
#include <uk/sys.h>
#include <uk/pfndb.h>
#include <machine/uk/cpu.h>
#include <machine/uk/platform.h>
#include <uk/heap.h>
#include <uk/kern.h>
#include <lib/lib.h>

#define MAXPLTSIGS 512

struct pltsig {
	struct irqsig irqsig;
	 LIST_ENTRY(pltsig) list;
};

struct pltmap {
	vaddr_t va;
	 LIST_ENTRY(pltmap) list;
};

struct pltrems {
	unsigned in_use:1;
	 LIST_HEAD(, pltsig) pltsigs;
	 LIST_HEAD(, pltmap) pltmaps;
};

static lock_t platform_lock = 0;
static struct dev platform_dev;
static struct pltrems platform_rems[MAXPLTSIGS];

int _pltdev_open(void *devopq, uint64_t did)
{
	int i, ret;

	spinlock(&platform_lock);
	for (i = 0; i < MAXPLTSIGS; i++)
		if (!platform_rems[i].in_use)
			break;
	if (i >= MAXPLTSIGS) {
		ret = -ENFILE;
		goto out;
	}
	LIST_INIT(&platform_rems[i].pltsigs);
	platform_rems[i].in_use = 1;
	ret = 0;
      out:
	spinunlock(&platform_lock);
	return ret;
}

static int _pltdev_in(void *devopq, unsigned id, uint64_t port,
		      uint64_t * val)
{

	switch (port & PLTPORT_SIZEMASK) {
	case 1:
		*val = platform_inb(port >> PLTPORT_SIZESHIFT);
		break;
	case 2:
		*val = platform_inw(port >> PLTPORT_SIZESHIFT);
		break;
	case 4:
		*val = platform_inl(port >> PLTPORT_SIZESHIFT);
		break;
	default:
		printf("Meh: %llx\n", port);
		return -EINVAL;
	}
	return 0;
}

static int _pltdev_out(void *devopq, unsigned id, uint64_t port,
		       uint64_t val)
{
	switch (port & PLTPORT_SIZEMASK) {
	case 1:
		platform_outb(port >> PLTPORT_SIZESHIFT, val);
		break;
	case 2:
		platform_outw(port >> PLTPORT_SIZESHIFT, val);
		break;
	case 4:
		platform_outl(port >> PLTPORT_SIZESHIFT, val);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int _pltdev_export(void *devopq, unsigned id, vaddr_t va,
			  unsigned iopfn)
{
	return -ENODEV;
}

static int _pltdev_iomap(void *devopq, unsigned id, vaddr_t va,
			 paddr_t mmioaddr, pmap_prot_t prot)
{
	pfn_t mmiopfn = (pfn_t)atop(mmioaddr);
	struct pltmap *pltmap;
	int ret;

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

	dprintf("mapping at addr %lx paddr %"PRIx64" (%d)\n",
	       va, (uint64_t)mmioaddr, prot);

	ret = iomap(va, mmiopfn, prot);
	if (ret < 0)
		return ret;

	pltmap = heap_alloc(sizeof(*pltmap));
	pltmap->va = va;
	spinlock(&platform_lock);
	LIST_INSERT_HEAD(&platform_rems[id].pltmaps, pltmap, list);
	spinunlock(&platform_lock);
	return 0;
}

static int _pltdev_iounmap(void *devopq, unsigned id, vaddr_t va)
{
	struct pltmap *pm, *tpm;

	spinlock(&platform_lock);
	assert(platform_rems[id].in_use);
	LIST_FOREACH_SAFE(pm, &platform_rems[id].pltmaps, list, tpm) {
		if (pm->va == va) {
			LIST_REMOVE(pm, list);
			vmunmap(va);
			heap_free(pm);
			spinunlock(&platform_lock);
			return 0;
		}
	}
	spinunlock(&platform_lock);
	return -ENOENT;
}

static int _pltdev_rdcfg(void *devopq, unsigned id,
			 struct sys_creat_cfg *cfg)
{
	return -ENODEV;
}

static int _pltdev_irqmap(void *devopq, unsigned id, unsigned irq,
			  unsigned sig)
{
	int ret;
	struct thread *th = current_thread();
	struct pltsig *pltsig;

	pltsig = heap_alloc(sizeof(struct pltsig));
	printf("Registering irq %d at thread %p with signal %d\n", irq, th,
	       sig);
	ret = irqregister(&pltsig->irqsig, irq, th, sig,
			  0 /* No filter */ );
	if (ret < 0)
		return ret;
	spinlock(&platform_lock);
	LIST_INSERT_HEAD(&platform_rems[id].pltsigs, pltsig, list);
	spinunlock(&platform_lock);
	return ret;
}

static void _pltdev_close(void *devopq, unsigned id)
{
	struct pltsig *ps, *tps;
	struct pltmap *pm, *tpm;

	spinlock(&platform_lock);
	assert(platform_rems[id].in_use);
	LIST_FOREACH_SAFE(pm, &platform_rems[id].pltmaps, list, tpm) {
		LIST_REMOVE(pm, list);
		vmunmap(pm->va);
		heap_free(pm);
	}
	LIST_FOREACH_SAFE(ps, &platform_rems[id].pltsigs, list, tps) {
		LIST_REMOVE(ps, list);
		irqunregister(&ps->irqsig);
		heap_free(ps);
	}
	platform_rems[id].in_use = 0;
	spinunlock(&platform_lock);
}

static struct devops pltdev_ops = {
	.open = _pltdev_open,
	.close = _pltdev_close,
	.in = _pltdev_in,
	.out = _pltdev_out,
	.export = _pltdev_export,
	.iomap = _pltdev_iomap,
	.iounmap = _pltdev_iounmap,
	.rdcfg = _pltdev_rdcfg,
	.irqmap = _pltdev_irqmap,
};

void pltdev_init(void)
{
	memset(platform_rems, 0, sizeof(platform_rems));
	dev_init(&platform_dev, 0, NULL, &pltdev_ops, 0, 0, 600);
	dev_attach(&platform_dev);
}
