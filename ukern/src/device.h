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


#ifndef __device_h
#define __device_h

#define MAXDEVINTRS 16

#include <uk/types.h>
#include <machine/uk/pmap.h>
#include <uk/locks.h>
#include <uk/rbtree.h>

#define DEVCTL_ENABLED (1L << 1)
#define DEVCTL_NOSIG   (1L << 2)

#define DEVSTATUS_IOPEND (1L << 1)

struct devdesc {
	lock_t lock;
	struct device *dev;
	struct thread *th;

	uint8_t devctl;
	uint8_t devstatus;
	unsigned intmap[MAXDEVINTRS];
	uint64_t io_port;
	uint64_t io_val;

	LIST_ENTRY(devdesc) list;
};

struct device {
	lock_t lock;
	uint64_t id;

	struct thread *th;
	unsigned signo;

	struct rb_node rb_node;
	LIST_HEAD(, devdesc) descs;
};

/* Process Side */
struct devdesc *device_open(uint64_t id);
int device_io(struct devdesc *dd, uint64_t port, uint64_t val);
int device_intmap(struct devdesc *dd, unsigned id, unsigned sig);

/* Device Side */
struct device *device_creat(uint64_t id, unsigned);

void device_free(struct device *dv);
void devices_init(void);

#endif /* __device_h */
