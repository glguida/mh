#include <uk/types.h>
#include <uk/assert.h>
#include <uk/locks.h>
#include <machine/uk/pmap.h>
#include <machine/uk/pae.h>
#include <machine/uk/tlb.h>
#include <ukern/fixmems.h>
#include <ukern/structs.h>
#include <lib/lib.h>

struct slab pmap_cache;
l1e_t *pmap_kernel_l1;

uintptr_t __getpdptr(void);
__regparm void __setpdptr(void *);

#define DEUKERNBASE(_p) ((uintptr_t)(_p) - UKERNBASE)

#define pmap_current()						\
    ((struct pmap *)((uintptr_t)UKERNBASE + __getpdptr()	\
		     - offsetof(struct pmap, pdptr)))

struct pmap *
pmap_alloc(void)
{
    l1e_t *l1;
    struct pmap *pmap;

    l1 = (l1e_t *)alloc4k();
    if (l1 == NULL)
	return NULL;

    pmap = structs_alloc(&pmap_cache);
    if (pmap == NULL) {
	free4k(l1);
	return NULL;
    }

    memset(l1, 0, 4096);
    /* (l1 + LINOFF) must always be the copy of PDPTR */
    l1[LINOFF + 2] = mklinl1e(DEUKERNBASE(l1), 
			      PG_A | PG_D | PG_W | PG_P);
    l1[LINOFF + 3] = mklinl1e(DEUKERNBASE(pmap_kernel_l1), 
			      PG_A | PG_D | PG_W | PG_P);

    pmap->pdptr[0] = 0;
    pmap->pdptr[1] = 0;
    pmap->pdptr[2] = mkl2e(DEUKERNBASE(l1), PG_P);
    pmap->pdptr[3] = mkl2e(DEUKERNBASE(pmap_kernel_l1), PG_P);

    pmap->lock = 0;
    pmap->refcnt = 0;

    return pmap;
}

static void
__setl1e(l1e_t *l1p, l1e_t l1e)
{
    volatile uint32_t *ptr = (volatile uint32_t *)l1p;

    *ptr = 0;
    *++ptr = l1e >> 32;
    *--ptr = l1e & 0xffffffff;
}

void
pmap_setl1e(struct pmap *pmap, vaddr_t va, l1e_t nl1e)
{
    l1e_t ol1e, *l1p;

    if (pmap == NULL) {
	pmap = pmap_current();
	l1p = __val1tbl(va) + L1OFF(va);
    } else
	panic("set to different pmap not supported yet.");

    spinlock(&pmap->lock);
    ol1e = *l1p;
    pmap->tlbflush = __tlbflushp(ol1e, nl1e);
    __setl1e(l1p, nl1e);
    spinunlock(&pmap->lock);
}

void
pmap_commit(struct pmap *pmap)
{

    if (pmap == NULL)
	pmap = pmap_current();

    spinlock(&pmap->lock);
    if (pmap->tlbflush & TLBF_GLOBAL)
	__flush_tlbs(-1, TLBF_GLOBAL);
    else if (pmap->tlbflush & TLBF_LOCAL)
	__flush_tlbs(pmap->cpumap, TLBF_LOCAL);
    pmap->tlbflush = 0;
    spinunlock(&pmap->lock);
}

struct pmap*
pmap_boot(void)
{
    struct pmap *bpmap;

    /* Safe now to alloc a pmap */
    bpmap =  pmap_alloc();
    if (bpmap == NULL)
	panic("BPMAP: FAILED");

    /* Set pmap current */
    /* No need for per-cpu pointer, exactly what CR3 is, minus
       the offset of pdptr in pmap. Incidentally, it is zero now. */
    bpmap->refcnt++;
    __setpdptr(bpmap->pdptr);

    return bpmap;
}

void
pmap_init(void)
{
    int i;
    l1e_t *kl1;

    setup_structcache(&pmap_cache, pmap);

    /* Initialise kernel pagetable */
    pmap_kernel_l1 = alloc4k();
    if (pmap_kernel_l1 < 0)
	panic("BOOTL1: Alloc failed");

    kl1 = (void *)pmap_kernel_l1;
    for (i = KL1_SDMAP; i < KL1_EDMAP; i++)
	kl1[i] = mkl1e((i * PAGE_SIZE), PG_A | PG_D | PG_W | PG_P);
    for (; i < NPTES; i++)
	kl1[i] = 0;

    /* Safe now to alloc a pmap */
}
