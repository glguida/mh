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


#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/rbtree.h>
#include <sys/bitops.h>
#include <machine/vmparam.h>
#include <machine/param.h>
#include <machine/mrgparam.h>
#include <assert.h>
#include "mrg.h"


/*
 * VM map db.
 */

static int initialized = 0;
static rb_tree_t vmap_rbtree;
static size_t vmap_size;

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
	free(vme);
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

	vme = malloc(sizeof(*vme));
	assert(vme != NULL);
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

static struct zone vmap_zone;

static void vmap_init(void)
{
	rb_tree_init(&vmap_rbtree, &vmap_tree_ops);
	vmapzone_init(&vmap_zone);
	vmap_size = 0;

	printf("Freeing from %lx to %lx\n",
	       VM_MINMMAP_ADDRESS, VM_MAXMMAP_ADDRESS);
	vmapzone_free(&vmap_zone, VM_MINMMAP_ADDRESS,
		      VM_MAXMMAP_ADDRESS - VM_MINMMAP_ADDRESS);
}

vaddr_t vmap_alloc(size_t size, uint8_t type)
{
	size_t pgsz;
	vaddr_t va;

	if (!initialized) {
		vmap_init();
		initialized++;
	}
	pgsz = round_page(size);
	va = vmapzone_alloc(&vmap_zone, pgsz);
	if (va != 0)
		vmap_insert(va, pgsz, type);
	return va;
}

void vmap_free(vaddr_t va, size_t size)
{
	struct vme *vme;

	if (!initialized) {
		vmap_init();
		initialized++;
	}
	size = round_page(size);
	vme = vmap_find(va);
	assert(vme != NULL);
	assert(vme->addr == va);
	assert(vme->size == size);
	assert(vme->type != VFNT_FREE);
	vmap_remove(vme);
	vmapzone_free(&vmap_zone, va, size);
}

void
vmap_info(vaddr_t va, vaddr_t * startp, size_t * sizep, uint8_t * typep)
{
	struct vme *vme = NULL;
	vaddr_t start;
	size_t size;
	uint8_t type;

	if (initialized) {
		vme = vmap_find(va);
	}
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
