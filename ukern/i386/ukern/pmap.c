#include <uk/types.h>
#include <machine/uk/pmap.h>
#include <ukern/fixmems.h>
#include <ukern/structs.h>
#include <lib/lib.h>

struct slab pmap_cache;
l1e_t *pmap_kernel_l1;

__regparm void __setpdptr(void *);

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
    l1[LINOFF + 2] = mklinl1e(l1, PG_A | PG_D | PG_W | PG_P);
    l1[LINOFF + 3] = mklinl1e(pmap_kernel_l1, PG_A | PG_D | PG_W | PG_P);

    pmap->pdptr[0] = 0;
    pmap->pdptr[1] = 0;
    pmap->pdptr[2] = mkl2e(l1, PG_P);
    pmap->pdptr[3] = mkl2e(pmap_kernel_l1, PG_P);

    pmap->lock = 0;
    pmap->refcnt = 0;

    return pmap;
}

struct pmap*
pmap_boot(void)
{
    int i;
    struct pmap *bpmap;
    l1e_t *kl1;

    setup_structcache(&pmap_cache, pmap);

    /* Initialise kernel pagetable */
    pmap_kernel_l1 = alloc4k();
    if (pmap_kernel_l1 < 0)
	panic("BOOTL1: Alloc failed");

    kl1 = (void *)pmap_kernel_l1;
    for (i = KL1_SDMAP; i < KL1_EDMAP; i++)
	kl1[i] = mkl1e(UKERNBASE + (i * PAGE_SIZE), PG_A | PG_D | PG_W | PG_P);
    for (; i < NPTES; i++)
	kl1[i] = 0;

    /* Safe now to alloc a pmap */
    bpmap =  pmap_alloc();
    if (bpmap == NULL)
	panic("BPMAP: FAILED");

    /* Set pmap current */
    /* XXX: No need for per-cpu pointer, exactly what CR3 is, since
       the offset of pdptr in pmap. Incidentally, it is zero now. */
    bpmap->refcnt++;
    __setpdptr(bpmap->pdptr);

    return bpmap;
}