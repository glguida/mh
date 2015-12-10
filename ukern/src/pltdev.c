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
#include <machine/uk/cpu.h>
#include <machine/uk/platform.h>
#include <uk/structs.h>
#include <uk/kern.h>
#include <lib/lib.h>

#define MAXPLTSIGS 512

struct pltsig {
	struct irqsig irqsig;
	LIST_ENTRY(pltsig) list;
};

struct pltrems {
	unsigned in_use :1;
	LIST_HEAD(, pltsig) pltsigs;
};

static struct slab platsigs;
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

static int _pltdev_in(void *devopq, unsigned id, uint64_t port, uint64_t *val)
{

	switch (port & PLTPORT_SIZEMASK) {
	case 1:
		printf("Byte input %llx\n", port);
		*val = platform_inb(port >> PLTPORT_SIZESHIFT);
		break;
	case 2:
		printf("Word input %llx\n", port);
		*val = platform_inw(port >> PLTPORT_SIZESHIFT);
		break;
	case 4:
		printf("Word input %llx\n", port);
		*val = platform_inl(port >> PLTPORT_SIZESHIFT);
		break;
	default:
		printf("Meh: %llx\n", port);
		return -EINVAL;
	}
	return 0;
}

static int _pltdev_out(void *devopq, unsigned id, uint64_t port, uint64_t val)
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

static int _pltdev_export(void *devopq, unsigned id, vaddr_t va, unsigned iopfn)
{
	return -ENODEV;
}

static int _pltdev_irqmap(void *devopq, unsigned id, unsigned irq, unsigned sig)
{
	struct thread *th = current_thread();
	struct pltsig *pltsig;

	pltsig = structs_alloc(&platsigs);
	printf("Registering irq %d at thread %p with signal %d\n", irq, th, sig);
	spinlock(&platform_lock);
	LIST_INSERT_HEAD(&platform_rems[id].pltsigs, pltsig, list);
	spinunlock(&platform_lock);
	return irqregister(&pltsig->irqsig, irq, th, sig);
}

static void _pltdev_close(void *devopq, unsigned id)
{
	struct pltsig *ps, *tps;

	spinlock(&platform_lock);
	assert(platform_rems[id].in_use);

	LIST_FOREACH_SAFE(ps, &platform_rems[id].pltsigs, list, tps) {
		LIST_REMOVE(ps, list);
		irqunregister(&ps->irqsig);
		structs_free(ps);
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
	.irqmap = _pltdev_irqmap,
};

void pltdev_init(void)
{
	setup_structcache(&platsigs, pltsig);
	memset(platform_rems, 0, sizeof(platform_rems));
	dev_init(&platform_dev, 0, NULL, &pltdev_ops, 0, 0, 600);
	dev_attach(&platform_dev);
}
