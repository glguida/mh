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


#ifndef _sys_dirtio_ring_h
#define _sys_dirtio_ring_h

/*
 * The following is the interface for a dirtio queue implementation,
 * which is used to safely pass buffers from processes to devices.
 *
 * This mimicks very closely virtio's virtqueues. Differences with
 * virtqueues are:
 *
 * 1. A single buffer entry cannot be longer than PAGE_SIZE.
 *
 * 2. Address must be an I/O address, pointing to addresses in the
 *    aperture table shared between the process and device.
 *
 * Following code is inherited from Linux's include/uapi/virtio_ring.h
 */

/* An interface for efficient virtio implementation, currently for use by KVM
 * and lguest, but hopefully others soon.  Do NOT change this since it will
 * break existing servers and clients.
 *
 * This header is BSD licensed so anyone can use the definitions to implement
 * compatible drivers/servers.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of IBM nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright Rusty Russell IBM Corporation 2007. */

#include <sys/dirtio_types.h>

/* This marks a buffer as continuing via the next field. */
#define DRING_DESC_F_NEXT	1
/* This marks a buffer as write-only (otherwise read-only). */
#define DRING_DESC_F_WRITE	2
/* This means the buffer contains a list of buffer descriptors. */
#define DRING_DESC_F_INDIRECT	4

/* The device uses this in used->flags to advise the process: don't
 * kick me when you add a buffer.  It's unreliable, so it's simply an
 * optimization.  Process will still kick if it's out of buffers. */
#define DRING_USED_F_NO_NOTIFY	1
/* The Process uses this in avail->flags to advise the Device: don't
 * interrupt me when you consume a buffer.  It's unreliable, so it's
 * simply an optimization.  */
#define DRING_AVAIL_F_NO_INTERRUPT	1

/* We support indirect buffer descriptors */
#define DIRTIO_RING_F_INDIRECT_DESC	28

/* The Process publishes the used index for which it expects an
 * interrupt at the end of the avail ring. Device should ignore the
 * avail->flags field. */
/* The Device publishes the avail index for which it expects a kick at
 * the end of the used ring. Process should ignore the used->flags
 * field. */
#define DIRTIO_RING_F_EVENT_IDX		29

/* Dirtio ring descriptors: 16 bytes.  These can chain together via "next". */
struct dring_desc {
	/* I/O Address (aperture). */
	__dirtio64 addr;
	/* Length. */
	__dirtio32 len;
	/* The flags as indicated above. */
	__dirtio16 flags;
	/* We chain unused descriptors via this, too */
	__dirtio16 next;
};

struct dring_avail {
	__dirtio16 flags;
	__dirtio16 idx;
	__dirtio16 ring[];
};

/* u32 is used here for ids for padding reasons. */
struct dring_used_elem {
	/* Index of start of used descriptor chain. */
	__dirtio32 id;
	/* Total length of the descriptor chain which was used (written to) */
	__dirtio32 len;
};

struct dring_used {
	__dirtio16 flags;
	__dirtio16 idx;
	struct dring_used_elem ring[];
};

struct dring {
	unsigned int num;

	struct dring_desc *desc;

	struct dring_avail *avail;

	struct dring_used *used;
};

/* Alignment requirements for dring elements.
 * When using pre-virtio 1.0 layout, these fall out naturally.
 */
#define DRING_AVAIL_ALIGN_SIZE 2
#define DRING_USED_ALIGN_SIZE 4
#define DRING_DESC_ALIGN_SIZE 16

/* The standard layout for the ring is a continuous chunk of memory which looks
 * like this.  We assume num is a power of 2.
 *
 * struct dring
 * {
 *	// The actual descriptors (16 bytes each)
 *	struct dring_desc desc[num];
 *
 *	// A ring of available descriptor heads with free-running index.
 *	__dirtio16 avail_flags;
 *	__dirtio16 avail_idx;
 *	__dirtio16 available[num];
 *	__dirtio16 used_event_idx;
 *
 *	// Padding to the next align boundary.
 *	char pad[];
 *
 *	// A ring of used descriptor heads with free-running index.
 *	__dirtio16 used_flags;
 *	__dirtio16 used_idx;
 *	struct dring_used_elem used[num];
 *	__dirtio16 avail_event_idx;
 * };
 */
/* We publish the used event index at the end of the available ring, and vice
 * versa. They are at the end for backwards compatibility. */
#define dring_used_event(vr) ((vr)->avail->ring[(vr)->num])
#define dring_avail_event(vr) (*(__dirtio16 *)&(vr)->used->ring[(vr)->num])

static inline void dring_init(struct dring *vr, unsigned int num, void *p,
			      unsigned long align)
{
	vr->num = num;
	vr->desc = p;
	vr->avail = p + num*sizeof(struct dring_desc);
	vr->used = (void *)(((uintptr_t)&vr->avail->ring[num] + sizeof(__dirtio16)
		+ align-1) & ~(align - 1));
}

static inline unsigned dring_size(unsigned int num, unsigned long align)
{
	return ((sizeof(struct dring_desc) * num + sizeof(__dirtio16) * (3 + num)
		 + align - 1) & ~(align - 1))
		+ sizeof(__dirtio16) * 3 + sizeof(struct dring_used_elem) * num;
}

/* The following is used with USED_EVENT_IDX and AVAIL_EVENT_IDX */
/* Assuming a given event_idx value from the other side, if
 * we have just incremented index from old to new_idx,
 * should we trigger an event? */
static inline int dring_need_event(__dirtio16 event_idx, __dirtio16 new_idx, __dirtio16 old)
{
	/* Note: Xen has similar logic for notification hold-off
	 * in include/xen/interface/io/ring.h with req_event and req_prod
	 * corresponding to event_idx + 1 and new_idx respectively.
	 * Note also that req_event and req_prod in Xen start at 1,
	 * event indexes in dirtio start at 0. */
	return (__dirtio16)(new_idx - event_idx - 1) < (__dirtio16)(new_idx - old);
}

#endif
