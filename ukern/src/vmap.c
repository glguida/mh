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
#include <uk/rbtree.h>
#include <uk/assert.h>
#include <uk/param.h>
#include <machine/uk/pmap.h>
#include <uk/locks.h>
#include <uk/structs.h>
#include <uk/vmap.h>
#include <lib/lib.h>


/*
 * VM map db.
 */

static rb_tree_t vmap_rbtree;
static size_t vmap_size;
static struct slab vme_structs;

struct vme {
	struct rb_node rb_node;
	 LIST_ENTRY(vme) list;
	uint8_t type;
	vaddr_t addr;
	size_t size;
};

struct vmap {
	rb_tree_t rbtree;
	size_t size;
};

static void vmap_remove(struct vme *vme)
{

	/* ASSERT ISA(vme) XXX: */
	rb_tree_remove_node(&vmap_rbtree, (void *) vme);
	vmap_size -= vme->size;
	structs_free(vme);
}

static struct vme *vmap_find(vaddr_t va)
{
	struct vme;

	/* ASSERT ISA(vme returned) XXX: */
	return rb_tree_find_node(&vmap_rbtree, (void *) &va);
}

static struct vme *vmap_insert(vaddr_t start, size_t len, uint8_t type)
{
	struct vme *vme;

	vme = structs_alloc(&vme_structs);
	vme->addr = start;
	vme->size = len;
	vme->type = type;
	rb_tree_insert_node(&vmap_rbtree, (void *) vme);
	vmap_size += vme->size;
	return vme;
}

static int vmap_compare_key(void *ctx, const void *n, const void *key)
{
	const struct vme *vme = n;
	const vaddr_t va = *(const vaddr_t *) key;

	if (va < vme->addr)
		return 1;
	if (va > vme->addr + (vme->size - 1))
		return -1;
	return 0;
}

static int vmap_compare_nodes(void *ctx, const void *n1, const void *n2)
{
	const struct vme *vmap1 = n1;
	const struct vme *vmap2 = n2;

	/* Assert non overlapping */
	assert(vmap1->addr < vmap2->addr || vmap1->addr > vmap2->size);
	assert(vmap2->addr < vmap1->addr || vmap2->addr > vmap1->size);

	if (vmap1->addr < vmap2->addr)
		return -1;
	if (vmap1->addr > vmap2->addr)
		return 1;
	return 0;
}

static const rb_tree_ops_t vmap_tree_ops = {
	.rbto_compare_nodes = vmap_compare_nodes,
	.rbto_compare_key = vmap_compare_key,
	.rbto_node_offset = offsetof(struct vme, rb_node),
	.rbto_context = NULL
};


/*
 * VM allocator.
 */

#define ALLOCFUNC(...) vmapzone_##__VA_ARGS__
#define __ZENTRY vme
#define __ZADDR_T vaddr_t

static void ___get_neighbors(vaddr_t addr, size_t size,
			     struct vme **pv, struct vme **nv)
{
	vaddr_t end = addr + size;
	struct vme *pvme = NULL, *nvme = NULL;

	if (addr == 0)
		goto _next;

	pvme = vmap_find(addr - 1);
	if (pvme != NULL && pvme->type == VFNT_FREE)
		*pv = pvme;

      _next:
	nvme = vmap_find(end);
	if (nvme != NULL && nvme->type == VFNT_FREE)
		*nv = nvme;
}

static struct vme *___mkptr(vaddr_t addr, size_t size)
{

	return vmap_insert(addr, size, VFNT_FREE);
}

static void ___freeptr(struct vme *vme)
{

	vmap_remove(vme);
}

#include "alloc.c"

static lock_t vmap_lock = 0;
static struct zone vmap_zone;

vaddr_t vmap_alloc(size_t size, uint8_t type)
{
	size_t pgsz;
	vaddr_t va;

	pgsz = round_page(size);
	spinlock(&vmap_lock);
	va = vmapzone_alloc(&vmap_zone, pgsz);
	spinunlock(&vmap_lock);
	if (va == 0)
		panic("OOVM");

	return va;
}

void vmap_free(vaddr_t va, size_t size)
{

	size = round_page(size);
	spinlock(&vmap_lock);
	vmapzone_free(&vmap_zone, va, size);
	spinunlock(&vmap_lock);
}

void
vmap_info(vaddr_t va, vaddr_t * startp, size_t * sizep, uint8_t * typep)
{
	struct vme *vme;
	vaddr_t start;
	size_t size;
	uint8_t type;

	spinlock(&vmap_lock);
	vme = vmap_find(va);
	spinunlock(&vmap_lock);
	if (vme == NULL) {
		start = 0;
		size = 0;
		type = VFNT_INVALID;
	} else {
		start = vme->addr;
		size = vme->size;
		type = vme->type;
	}

	if (startp)
		*startp = start;
	if (sizep)
		*sizep = size;
	if (typep)
		*typep = type;
}

void vmap_init(void)
{

	setup_structcache(&vme_structs, vme);
	rb_tree_init(&vmap_rbtree, &vmap_tree_ops);
	vmapzone_init(&vmap_zone);
	vmap_lock = 0;
	vmap_size = 0;
}


/*
 * Global kernel memory mapper.
 */

void *kvmap(paddr_t addr, size_t size)
{
	int rc;
	size_t i;
	vaddr_t lobits;
	vaddr_t vaddr;
	paddr_t start, end;


	assert(size != 0);
	lobits = addr & PAGE_MASK;
	start = trunc_page(addr);
	end = round_page(addr + size);

	vaddr = vmap_alloc(end - start, VFNT_MAP);

	for (i = 0; i < (end - start) >> PAGE_SHIFT; i++) {
		pfn_t pfn;
		rc = pmap_kenter(NULL, vaddr + (i << PAGE_SHIFT),
				 atop(addr) + i,
				 PROT_GLOBAL | PROT_KERNWR, &pfn);
		/* We do not own the page mapped in the kvmap, do
		 * nothing with unmapped pfns. */
		assert(!rc && "pmap_kenter in kvmap");
	}
	pmap_commit(NULL);

	return (void *) (vaddr + lobits);
}

void kvunmap(vaddr_t vaddr, size_t size)
{
	int rc;
	size_t i;
	vaddr_t start, end;

	start = trunc_page(vaddr);
	end = round_page(vaddr + size);

	for (i = 0; i < (end - start) >> PAGE_SHIFT; i++) {
		pfn_t pfn;
		rc = pmap_kclear(NULL, vaddr + (i << PAGE_SHIFT), &pfn);
		assert(!rc && "pmap_kclear in kvunmap");
		/* We do not own pages we map. Ignore pfn */
	}
	pmap_commit(NULL);

	vmap_free(start, end - start);
}
