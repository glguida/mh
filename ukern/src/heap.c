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
#include <uk/param.h>
#include <uk/assert.h>
#include <uk/locks.h>
#include <uk/pfndb.h>
#include <lib/lib.h>

#define LIST_PREV(elm, type, field) ((elm)->field.le_prev ?		\
			      (struct type *)((char *)(elm)->field.le_prev - \
					      offsetof(struct type, field)) \
			      : NULL)

struct ekheap {
	int isfree;
	size_t ulen;
	 LIST_ENTRY(ekheap) list;
	 LIST_ENTRY(ekheap) free_list;
};

struct kheap {
	lock_t lock;
	 LIST_ENTRY(kheap) kheaps;
	 LIST_HEAD(, ekheap) list;
	 LIST_HEAD(, ekheap) free_list;
};

lock_t kheaps_lock;
LIST_HEAD(, kheap) kheaps;

#define UNITS(_s) ((_s) / sizeof(struct ekheap))
#define UROUND(_s) (UNITS((_s) + sizeof(struct ekheap) - 1))
#define HEAP_MAXALLOC (UNITS(PAGE_SIZE) - 1)

static struct kheap *kheap_add(void)
{
	pfn_t pfn = __allocpage(PFNT_HEAP);
	struct kheap *hptr;
	struct ekheap *eptr;

	/* Lock free, not connected */
	hptr = pfndb_getptr(pfn);
	hptr->lock = 0;
	LIST_INIT(&hptr->list);
	LIST_INIT(&hptr->free_list);

	eptr = (struct ekheap *) ptova(pfn);
	eptr->isfree = 1;
	eptr->ulen = UNITS(PAGE_SIZE);
	LIST_INSERT_HEAD(&hptr->list, eptr, list);
	LIST_INSERT_HEAD(&hptr->free_list, eptr, free_list);
	return hptr;
}

static void *kheap_alloc(struct kheap *hptr, size_t sz)
{
	struct ekheap *eptr, *neptr;
	unsigned udiff;
	unsigned units = UROUND(sz) + 1;	/* Account for hdr */

	/* Splintering the heap is atomic. Long spinlock */
	spinlock(&hptr->lock);
	LIST_FOREACH(eptr, &hptr->free_list, free_list)
		if (eptr->ulen >= units)
		break;

	if (eptr == LIST_END(&hptr->free_list)) {
		spinunlock(&hptr->lock);
		return NULL;
	}

	assert(eptr->isfree);

	udiff = eptr->ulen - units;
	if (udiff) {
		neptr = eptr + units;
		neptr->isfree = 1;
		neptr->ulen = udiff;

		LIST_INSERT_AFTER(eptr, neptr, list);
		LIST_INSERT_HEAD(&hptr->free_list, neptr, free_list);
	}

	LIST_REMOVE(eptr, free_list);
	eptr->isfree = 0;
	eptr->ulen = units;
	spinunlock(&hptr->lock);

	return eptr + 1;
}

static void kheap_free(void *ptr)
{
	unsigned pfn = vatop(ptr);
	struct kheap *hptr;
	struct ekheap *eptr, *peptr, *neptr;

	assert(pfndb_type(pfn) == PFNT_HEAP);
	hptr = (struct kheap *) pfndb_getptr(pfn);
	eptr = (struct ekheap *) ptr - 1;

	spinlock(&hptr->lock);
	peptr = LIST_PREV(eptr, ekheap, list);
	neptr = LIST_NEXT(eptr, list);
	if (neptr && neptr->isfree) {
		neptr->isfree = 0;
		LIST_REMOVE(neptr, free_list);
		LIST_REMOVE(neptr, list);
		eptr->ulen += neptr->ulen;
	}
	if (peptr && peptr->isfree) {
		LIST_REMOVE(eptr, list);
		peptr->ulen += eptr->ulen;
	} else {
		eptr->isfree = 1;
		LIST_INSERT_HEAD(&hptr->free_list, eptr, free_list);
	}
	spinunlock(&hptr->lock);
}

void *heap_alloc(size_t size)
{
	void *ptr = NULL;
	struct kheap *hptr;

	if (size >= HEAP_MAXALLOC) {
		panic("heap: size too big");
		return NULL;
	}

	spinlock(&kheaps_lock);
	LIST_FOREACH(hptr, &kheaps, kheaps)
		if ((ptr = kheap_alloc(hptr, size)) != NULL)
		break;
	spinunlock(&kheaps_lock);
	if (ptr != NULL)
		return ptr;

	hptr = kheap_add();
	ptr = kheap_alloc(hptr, size);

	spinlock(&kheaps_lock);
	LIST_INSERT_HEAD(&kheaps, hptr, kheaps);
	spinunlock(&kheaps_lock);

	return ptr;
}

void heap_free(void *ptr)
{

	kheap_free(ptr);
}

void heap_init(void)
{

	assert(sizeof(struct kheap) < sizeof(ipfn_t));

	kheaps_lock = 0;
	LIST_INIT(&kheaps);
}

void heap_shrink(void)
{

#warning implement me
}
