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


#ifndef __bus_h
#define __bus_h

#include <uk/types.h>
#include <uk/locks.h>
#include <uk/queue.h>
#include <uk/rbtree.h>

#define MAXBUSDEVS 64

struct bus {
	lock_t lock;
	struct bdeve {
		int bsy:1;	/* Currently being used (open) */
		int plg:1;	/* Device not unplugged */
		unsigned devid;
		struct bus *bus;
		struct dev *dev;
		 LIST_ENTRY(bdeve) list;
	} devs[MAXBUSDEVS];
};

struct sys_creat_cfg;
struct devops {
	int (*open) (void *devopq, uint64_t did);
	int (*in) (void *devopq, unsigned id, uint64_t port,
		   uint64_t * val);
	int (*out) (void *devopq, unsigned id, uint64_t port,
		    uint64_t val);
	int (*iomap) (void *devopq, unsigned id, vaddr_t va,
		      paddr_t mmioaddr, pmap_prot_t prot);
	int (*iounmap) (void *devopq, unsigned id, vaddr_t va);
	int (*export) (void *devopq, unsigned id, vaddr_t va,
		       unsigned iopfn);
	int (*rdcfg) (void *devopq, unsigned id,
		      struct sys_creat_cfg * cfg);
	int (*irqmap) (void *devopq, unsigned id, unsigned intr,
		       unsigned sig);
	void (*close) (void *devopq, unsigned id);
};

struct dev {
	uint64_t did;
	lock_t lock;
	int offline:1;

	uid_t uid;
	gid_t gid;
	devmode_t mode;

	void *devopq;
	struct devops *ops;
	 LIST_HEAD(, bdeve) busdevs;
	struct rb_node rb_node;
};

#define BUS_IRQEIO 0		/* Raised at EIO. */

int bus_plug(struct bus *b, uint64_t did);
int bus_in(struct bus *b, unsigned desc, uint64_t port, uint64_t * valptr);
int bus_out(struct bus *b, unsigned desc, uint64_t port, uint64_t val);
int bus_export(struct bus *b, unsigned desc, vaddr_t va, unsigned iopfn);
int bus_iomap(struct bus *b, unsigned desc, vaddr_t va,
	      paddr_t mmioaddr, pmap_prot_t prot);
int bus_irqmap(struct bus *b, unsigned desc, unsigned intr, unsigned sig);
int bus_unplug(struct bus *b, unsigned desc);
int bus_rdcfg(struct bus *b, unsigned desc, struct sys_creat_cfg *cfg);
void bus_remove(struct bus *b);

void dev_init(struct dev *d, uint64_t id, void *opq, struct devops *ops,
	      uid_t uid, gid_t gid, devmode_t mode);
int dev_attach(struct dev *d);
void dev_detach(struct dev *d);
void dev_free(struct dev *d);

void devices_init(void);

#endif
