#include <uk/types.h>
#include <uk/assert.h>
#include <uk/rbtree.h>
#include <uk/locks.h>
#include <uk/elf.h>
#include <machine/uk/cpu.h>
#include <machine/uk/pmap.h>
#include <ukern/pfndb.h>
#include <ukern/thread.h>
#include <ukern/structs.h>
#include <ukern/pgalloc.h>
#include <ukern/usrentry.h>
#include <ukern/addrspc.h>
#include <lib/lib.h>

static struct slab addrspcs;
static struct slab vranges;

static int
__vrange_cmp_key(void *ctx, const void *n, const void *key)
{
    const struct vrange *vr = n;
    const vaddr_t va = *(const vaddr_t *)key;

    if (va < vr->start)
	return -1;
    if (va > vr->end)
	return 1;
    return 0;
}

static int
__vrange_cmp_nodes(void *ctx, const void *n1, const void *n2)
{
    const struct vrange *vr1 = n1;
    const struct vrange *vr2 = n2;

    if (vr1->start > vr2->start)
	return -1;
    if (vr1->start < vr2->start)
	return 1;
    return 0;
}

static const rb_tree_ops_t vrange_ops = {
    .rbto_compare_nodes = __vrange_cmp_nodes,
    .rbto_compare_key = __vrange_cmp_key,
    .rbto_node_offset = offsetof(struct vrange, rb_node),
    .rbto_context = NULL
};

void
addrspc_init(void)
{

    setup_structcache(&addrspcs, addrspc);
    setup_structcache(&vranges, vrange);
}

struct addrspc *
addrspc_boot(struct pmap *pmap)
{
    struct addrspc *as;

    as = structs_alloc(&addrspcs);
    as->lock = 0;
    rb_tree_init(&as->vranges, &vrange_ops);
    as->pmap = pmap;
    return as;
}

struct addrspc *
addrspc_create(void)
{
    struct addrspc *as;

    as = structs_alloc(&addrspcs);
    as->pmap = pmap_alloc();
    as->lock = 0;
    rb_tree_init(&as->vranges, &vrange_ops);
    return as;
}

int
addrspc_add_range(struct addrspc *as, vaddr_t start, size_t size,
		  pmap_prot_t prot, struct pager *pgr, void *opq)
{
    int rc;
    struct vrange *vr = structs_alloc(&vranges);
    struct vrange *vk;

    vr->start = start;
    vr->end = start + size - 1;
    vr->prot = prot;
    vr->pager = pgr;
    vr->pager_opq = opq;

    spinlock(&as->lock);
    RB_TREE_FOREACH(vk, &as->vranges) {
      if (vk->end < vr->start)
	  continue;
      if (vk->start > vr->end)
	  break;

      if (((vk->start >= vr->start) && (vk->start <= vr->end))
	  || ((vr->start >= vk->start) && (vr->start <= vk->end))) {
	  /* Overlapping entry */
	  rc = -1;
	  goto out;
      }
    }
    rb_tree_insert_node(&as->vranges, (void *)vr);
    rc = 0;
  out:
    spinunlock(&as->lock);

    return rc;
}

struct pager *
addrspc_get_pager(struct addrspc *as, vaddr_t va)
{
    struct vrange *vr;

    spinlock(&as->lock);
    vr = (struct vrange *)rb_tree_find_node(&as->vranges, (void *)&va);
    spinunlock(&as->lock);
    if (vr == NULL)
	return NULL;

    return vr->pager;
}

int
addrspc_pagefault(vaddr_t va, pmap_fault_t fault)
{
    struct vrange *vr;
    struct addrspc *as = current_thread()->addrspc;

    spinlock(&as->lock);
    vr = (struct vrange *)rb_tree_find_node(&as->vranges, (void *)&va);
    spinunlock(&as->lock);
    if (vr == NULL) {
	return -1;
    }

    if (vr->pager == NULL) {
	return -1;
    }

    return vr->pager->__pgin(vr, va - vr->start, fault, vr->pager_opq);
}

int __pager_fill_pgin(struct vrange *vr, size_t off, pmap_fault_t fault,
		      void *opq)
{
    pfn_t pfn;

    pfn = __allocpage(PFNT_SYSTEM /* XXX: */);
    memcpy((void *)ptova(pfn),
	   (char *)opq + off, vr->end - vr->start + 1);

    pmap_enter(NULL, vr->start + off,
	       ptoa(pfn), vr->prot);
    pmap_commit(NULL);
    return 0;
}

size_t __pager_fill_pgout(struct vrange *vr, size_t sz, void *opq)
{

    /* XXX: ? */
    return 0;
}

static struct pager __pager_fill = {
    .__pgin = __pager_fill_pgin,
    .__pgout = __pager_fill_pgout,
};

int __pager_bzero_pgin(struct vrange *vr, size_t off, pmap_fault_t fault,
		       void *opq)
{
    pfn_t pfn;

    pfn = __allocpage(PFNT_SYSTEM);
    memset((void *)ptova(pfn), 0, PAGE_SIZE);
    pmap_enter(NULL, vr->start + off, ptoa(pfn), vr->prot);
    pmap_commit(NULL);
    return 0;
}

size_t __pager_bzero_pgout(struct vrange *vr, size_t sz, void *opq)
{

    /* XXX: ? */
    return 0;
}

static struct pager __pager_bzero = {
    .__pgin = __pager_bzero_pgin,
    .__pgout = __pager_bzero_pgout,
};

void
addrspc_elf_load(struct addrspc *as, struct usrentry *ue, void *elfimg)
{
    int i;
    int rc = 0;
#define ELFOFF(_o) ((void *)((uintptr_t)elfimg + (_o)))

    char elfid[] = { 0x7f, 'E', 'L', 'F', };
    struct elfhdr __packed *hdr = (struct elfhdr *)elfimg;
    struct elfph *ph = (struct elfph *)ELFOFF(hdr->phoff);

    assert(!memcmp(hdr->id, elfid, 4));
    for (i = 0; i < hdr->phs; i++, ph++) {
	if (ph->type != PHT_LOAD)
	    continue;
	if (ph->fsize)
	  rc = addrspc_add_range(as, ph->va, ph->fsize, PROT_USER_WRX,
				 &__pager_fill, (void *)ELFOFF(ph->off));
	assert(rc == 0);
	if ((ph->msize - ph->fsize) > 0)
	    rc = addrspc_add_range(as, ph->va + ph->fsize,
				   ph->msize - ph->fsize,
				   PROT_USER_WR, &__pager_bzero, NULL);
	assert(rc == 0);
    }

    usrentry_setup(ue, hdr->entry);
}

