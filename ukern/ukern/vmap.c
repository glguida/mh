#include <uk/types.h>
#include <uk/rbtree.h>
#include <uk/assert.h>
#include <uk/param.h>
#include <machine/uk/pmap.h>
#include <ukern/structs.h>
#include <ukern/vmap.h>
#include <lib/lib.h>


/*
 * VM map db.
 */

static rb_tree_t   vmap_rbtree;
static size_t      vmap_size;
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

static void
vmap_remove(struct vme *vme)
{

    /* ASSERT ISA(vme) XXX: */
    rb_tree_remove_node(&vmap_rbtree, (void *)vme);
    vmap_size -= vme->size;
    structs_free(vme);
}

static struct vme *
vmap_find(vaddr_t va)
{
    struct vme;

    /* ASSERT ISA(vme returned) XXX: */
    return rb_tree_find_node(&vmap_rbtree, (void *)&va);
}

static struct vme *
vmap_insert(vaddr_t start, size_t len, uint8_t type)
{
    struct vme *vme;

    vme = structs_alloc(&vme_structs);
    vme->addr = start;
    vme->size = len;
    vme->type = type;
    rb_tree_insert_node(&vmap_rbtree, (void *)vme);
    vmap_size += vme->size;
    return vme;
}

static int
vmap_compare_key(void *ctx, const void *n, const void *key)
{
    const struct vme *vme = n;
    const vaddr_t va = *(const vaddr_t *)key;

    if (va < vme->addr)
	return 1;
    if (va > vme->addr + (vme->size - 1))
	return -1;
    return 0;
}

static int
vmap_compare_nodes(void *ctx, const void *n1, const void *n2)
{
    const struct vme *vmap1 = n1;
    const struct vme *vmap2 = n2;

    /* Assert non overlapping */
    assert(vmap1->addr < vmap2->addr
	   || vmap1->addr > vmap2->size);
    assert(vmap2->addr < vmap1->addr
	   || vmap2->addr > vmap1->size);

    if (vmap1->addr < vmap2->addr)
	return -1;
    if (vmap1->size > vmap2->addr)
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
		     struct vme **pv,
		     struct vme **nv)
{
    vaddr_t end  = addr + size;
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

static struct vme *
___mkptr(vaddr_t addr, size_t size)
{

    return vmap_insert(addr, size, VFNT_FREE);
}

static void
___freeptr(struct vme *vme)
{

    vmap_remove(vme);
}

#include "alloc.c"

static struct zone vmap_zone;

vaddr_t
vmap_alloc(size_t size, uint8_t type)
{
    size_t  pgsz;
    vaddr_t va;

    pgsz = round_page(size);
    va = vmapzone_alloc(&vmap_zone, pgsz);
    if (va == 0)
	panic("OOVM");

    return va;
}

void
vmap_free(vaddr_t va, size_t size)
{

    vmapzone_free(&vmap_zone, va, size);
}

void
vmap_info(vaddr_t va, vaddr_t *startp, size_t *sizep, uint8_t *typep)
{
    struct vme *vme;
    vaddr_t start;
    size_t size;
    uint8_t type;

    vme = vmap_find(va);
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

void
vmap_init(void)
{

    setup_structcache(&vme_structs,  vme);
    rb_tree_init(&vmap_rbtree, &vmap_tree_ops);
    vmapzone_init(&vmap_zone);
    vmap_size = 0;
}


/*
 * Global kernel memory mapper.
 */

void *kvmap(paddr_t addr, size_t size)
{
    size_t i;
    vaddr_t lobits;
    vaddr_t vaddr;
    paddr_t start, end;


    assert(size != 0);
    lobits = addr & PAGE_MASK;
    start = trunc_page(addr);
    end = round_page(addr + size);

    vaddr = vmap_alloc(end - start, VFNT_MAP);

    for (i = 0; i < (end - start) >> PAGE_SHIFT; i++)
	pmap_enter(NULL, vaddr + (i << PAGE_SHIFT),
		   addr + (i << PAGE_SHIFT), PROT_GLOBAL|PROT_KERNWR);
    pmap_commit(NULL);

    return (void *)(vaddr + lobits);
}

void kvunmap(vaddr_t vaddr, size_t size)
{
    size_t i;
    vaddr_t start, end;

    start = trunc_page(vaddr);
    end = round_page(vaddr + size);

    for (i = 0; i < (end - start) >> PAGE_SHIFT; i++)
	pmap_clear(NULL, vaddr + (i << PAGE_SHIFT));
    pmap_commit(NULL);

    vmap_free(start, end - start);
}
