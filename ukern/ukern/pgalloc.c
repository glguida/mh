#include <uk/types.h>
#include <uk/queue.h>
#include <uk/param.h>
#include <machine/uk/locks.h>
#include <ukern/pgalloc.h>
#include <ukern/pfndb.h>

#define __PFNMAX(_p) ((_p) < pfndb_max() ? (_p) : pfndb_max())

#define MEMTYPE_LOKERN_START    (LKMEMSTRT >> PAGE_SHIFT)
#define MEMTYPE_LOKERN_END      __PFNMAX(LKMEMEND >> PAGE_SHIFT)
#define MEMTYPE_KERN_START      __PFNMAX(KMEMSTRT >> PAGE_SHIFT)
#define MEMTYPE_KERN_END        __PFNMAX(KMEMEND >> PAGE_SHIFT)
#define MEMTYPE_HIGH_START      __PFNMAX(HIGHSTRT >> PAGE_SHIFT)
#define MEMTYPE_HIGH_END        pfndb_max()

#define PFNZ_LOKERN(_p)				\
    (((_p) >= MEMTYPE_LOKERN_START) &&		\
     ((_p) <  MEMTYPE_LOKERN_END))

#define PFNZ_KERN(_p)			  \
    (((_p) >= MEMTYPE_KERN_START) &&	  \
     ((_p) < MEMTYPE_KERN_END))

#define PFNZ_HIGH(_p)			\
    (((_p) >= MEMTYPE_HIGH_START) &&	\
     ((_p) < MEMTYPE_HIGH_END))


#define PFNZ_TYPE(_p)					\
    (PFNZ_KERN(_p) ? 1 : PFNZ_LOKERN(_p) ? 0 : 2)

#define NPFNZTYPES 3


/*
 * Allocator configuration.
 */

#define ALLOCFUNC(...) pgzone_##__VA_ARGS__
#define __ZENTRY pgzentry
#define __ZADDR_T pfn_t

#define is_pfnt_pgzfree(_t)			\
    ((_t) == PFNT_FREE				\
     || (_t) == PFNT_FREE_PZ_LONE		\
     || (_t) == PFNT_FREE_PZ_STRT		\
     || (_t) == PFNT_FREE_PZ_TERM)

#define is_pfnt_pgzterm(_t)		\
    ((_t) == PFNT_FREE_PZ_LONE		\
     || (_t) == PFNT_FREE_PZ_TERM)

#define is_pfnt_pgzstart(_t)		\
    ((_t) == PFNT_FREE_PZ_LONE		\
     || (_t) == PFNT_FREE_PZ_STRT)

#define is_pgzfree(_p)				\
    is_pfnt_pgzfree(pfndb_type(_p))

#define is_pgzterm(_p)				\
    is_pfnt_pgzterm(pfndb_type(_p))

#define is_pgzstart(_p)				\
    is_pfnt_pgzstart(pfndb_type(_p))



static void
___get_neighbors(pfn_t addr, size_t size,
		  struct pgzentry **pz,
		  struct pgzentry **nz)
{
    struct pgzentry *pze, *nze;

    if (addr == 0)
	goto _next;


    if (PFNZ_TYPE(addr) != PFNZ_TYPE(addr - 1))
	goto _next;

    pze = (struct pgzentry *)pfndb_getptr(addr - 1);
    if (is_pgzterm(addr - 1))
	*pz = pze;

  _next:
    if (PFNZ_TYPE(addr) != PFNZ_TYPE(addr + 1))
	return;

    nze = (struct pgzentry *)pfndb_getptr(addr + size);
    if (is_pgzstart(addr + size))
	*nz = nze;

}

static struct pgzentry *
___mkptr(pfn_t addr, size_t size)
{
    struct pgzentry *fze, *lze;

    fze = (struct pgzentry *)pfndb_getptr(addr);
    lze = (struct pgzentry *)pfndb_getptr(addr + size - 1);

    fze->addr = addr;
    fze->size = size;
    lze->addr = addr;
    lze->size = size;

    if (size == 1)
      pfndb_settype(addr, PFNT_FREE_PZ_LONE);
    else {
	int i;

	pfndb_settype(addr, PFNT_FREE_PZ_STRT);
	for (i = 1; i < size - 1; i++)
	    pfndb_settype(addr + i, PFNT_FREE);
	pfndb_settype(addr + i, PFNT_FREE_PZ_TERM);
    }

    return fze;
}

static void
___freeptr(struct pgzentry *ze)
{

    /* None */
}

#include "alloc.c"

/*
 * The multi-page allocator.
 */


lock_t pgalloc_lck[NPFNZTYPES];
struct zone pgzones[NPFNZTYPES];

#define __LCK(_i, ...) do {			\
	spinlock(pgalloc_lck + (_i));		\
	__VA_ARGS__;				\
	spinunlock(pgalloc_lck + (_i));		\
    } while(0)

pfn_t
pgalloc(size_t size, uint8_t type, unsigned long flags)
{
    int i;
    pfn_t addr = 0;
    assert(size != 0);

    if (flags == 0)
	flags = GFP_DEFAULT;

    if (flags & GFP_HIGH_ONLY)
	__LCK(2, {
		addr = pgzone_alloc(pgzones + 2, size);
	    });
    if (!addr && flags & GFP_KERN_ONLY)
	__LCK(1, { 
		addr = pgzone_alloc(pgzones + 1, size);
	    });
    if (!addr && flags & GFP_LOKERN_ONLY)
	__LCK(0, {
		addr = pgzone_alloc(pgzones + 0, size);
	    });
    if (!addr)
	panic("OOM");

    for (i = 0; i < size; i++)
	pfndb_settype(addr + i, type);

    return addr;
}

void
pgfree(pfn_t pfn, size_t size)
{
    int pfnz_type;
    assert(pfn != 0);
    assert(size != 0);

    pfnz_type = PFNZ_TYPE(pfn);
    assert(pfnz_type < NPFNZTYPES);

    __LCK(pfnz_type, {
	    pgzone_free(pgzones + pfnz_type, pfn, size);
	});
}

void
pginit(void)
{
    int j;
    pfn_t start, i;
    int status=0; 	/* 1: we're scanning freezone,
			 * 0: we're searching for free zone */

    for (j = 0; j < 3; j++) {
	pgalloc_lck[j] = 0;
	pgzone_init(pgzones + j);
    }

    start = 0;
    for (i = 0; i < pfndb_max(); i++) {
	if ((pfndb_type(i) == PFNT_FREE) && (status == 0)) {
	    status = 1;
	    start = i;
	    continue;
	}
	if ((pfndb_type(i) != PFNT_FREE) && (status == 1)) {
	    status = 0;
	    pgzone_free(pgzones + PFNZ_TYPE(start), start, i - start);
	    continue;
	}
	/* If we're crossing a memory type boundary, close 
	   the pgzone.  */
	if ((status == 1) &&
	    (pfndb_type(i) == PFNT_FREE) &&
	    (PFNZ_TYPE(i) != PFNZ_TYPE(i-1))) {
	    pgzone_free(pgzones + PFNZ_TYPE(start), start, i - start);
	    start = i;
	    continue;
	}
    }

    if (status == 1) {
	/* save last freezone */
	pgzone_free(pgzones + PFNZ_TYPE(start), start, i - start);
    }
}
