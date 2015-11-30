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

static struct slab usrdev_opaques;

struct usrdev_devopq {
	struct thread *th;
	unsigned sig;
};

static void *_usrdev_open(void *devopq, uint64_t did)
{
	printf("open %"PRIx64"!", did);
	return (void *)-1;
}

static void _usrdev_close(void *devopq, void *busopq)
{

	printf("close!\n");
}

static void _usrdev_io(void *devopq, void *busopq, uint64_t port, uint64_t val)
{
	struct usrdev_devopq *udo = (struct usrdev_devopq *)devopq;

	printf("io(%"PRIx64") = %"PRIx64"\n", port, val);
	thraise(udo->th, udo->sig);
}

static void _usrdev_intmap(void *devopq, void *busopq, unsigned intr, unsigned sig)
{
	printf("intmap(%x) = %x\n", intr, sig);
}

static struct devops usrdev_ops = {
	.open = _usrdev_open,
	.close = _usrdev_close,
	.io = _usrdev_io,
	.intmap = _usrdev_intmap,
};

static void *usrdev_opq(uint64_t id, unsigned sig)
{
	struct usrdev_devopq *udo;

	udo = structs_alloc(&usrdev_opaques);
	udo->th = current_thread();
	udo->sig = sig;
	return (void *)udo;
}

struct dev *usrdev_creat(uint64_t id, unsigned sig)
{
	struct dev *d;
	struct usrdev_devopq *udo;

	d = dev_alloc(id);
	if (d == NULL)
		return d;

	udo = structs_alloc(&usrdev_opaques);
	udo->th = current_thread();
	udo->sig = sig;
	d->devopq = (void *)udo;
	d->ops = &usrdev_ops;

	if (dev_attach(d)) {
		structs_free(d->devopq);
		dev_free(d);
		return NULL;
	}

	return d;
}

void usrdev_destroy(struct dev *d)
{
	dev_detach(d);
	structs_free(d->devopq);
	dev_free(d);
}

void usrdevs_init(void)
{
	setup_structcache(&usrdev_opaques, usrdev_devopq);
}
