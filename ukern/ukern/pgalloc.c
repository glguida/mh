#include <uk/types.h>
#include <uk/assert.h>
#include <uk/locks.h>
#include <ukern/pfndb.h>
#include <lib/lib.h>

#include "alloc.h"

#define ZONEFUNC(_f) static zone_##_f

#include "alloc.c"

static inline int is_zonestart(unsigned pfn)
{
    uint8_t type = pfndb_type(pfn);

    return type == PFNT_FREE_PZ_LONE || type == PFNT_FREE_PZ_STRT;
}

static inline int is_zoneterm(unsigned pfn)
{
    uint8_t type = pfndb_type(pfn);

    return type == PFNT_FREE_PZ_LONE || type == PFNT_FREE_PZ_TERM;
}

static inline int is_zonefree(unsigned pfn)
{
    uint8_t type = pfndb_type(pfn);

    return type == PFNT_FREE_PZ_LONE || type == PFNT_FREE_PZ_TERM
	|| type == PFNT_FREE_PZ_STRT || type == PFNT_FREE;
}

static struct zentry *
___create_zentry(struct zone *zn, uintptr_t addr, size_t len)
{
    int i;
    struct zentry *ze = (struct zentry *)pfndb_getptr(addr), *ztmp;

    dprintf("(creating entry %08lx:%08lx)\n", addr, addr+len);

    ze->addr = addr;
    ze->len = len;
    if (len == 1) {
	pfndb_settype(addr, PFNT_FREE_PZ_LONE);
	return ze;
    }

    ztmp = (struct zentry *)pfndb_getptr(addr + len - 1);
    ztmp->addr = addr;
    ztmp->len = len;

    pfndb_settype(addr, PFNT_FREE_PZ_STRT);
    for (i = 1; i < len - 1; i++)
	pfndb_settype(addr + i, PFNT_FREE);
    pfndb_settype(addr + len - 1, PFNT_FREE_PZ_TERM);
    return ze;
}

struct zentry *
___get_zentry_prev(struct zone *zn, uintptr_t addr, size_t len)
{
    unsigned pfn;

    dprintf("(check prev entry %08lx:%08lx)\n", addr, addr + len);
    
    pfn = addr - 1;
    if (is_zoneterm(pfn))
	return pfndb_getptr(pfn);
    return NULL;
}

struct zentry *
___get_zentry_next(struct zone *zn, uintptr_t addr, size_t len)
{
    unsigned pfn;

    dprintf("(check next entry %08lx:%08lx)\n", addr, addr + len);

    pfn = addr + len;
    if (is_zonestart(pfn))
	return pfndb_getptr(pfn);
    return NULL;
}

struct zentry *
___get_zentry(struct zone *zn, uintptr_t addr, size_t len)
{

    dprintf("(get entry %08lx:%08lx)\n", addr, addr + len);

    assert(is_zonestart(addr));
    return pfndb_getptr(addr);
}

void
___free_zentry(struct zone *zn, struct zentry *ze)
{

    dprintf("(freeing entry %08lx:%08lx)\n", ze->addr, ze->addr+ze->len);
}

/* XXX: MOVE CONSTANTS TO PARAM_H. */

#define MEMTYPE_LOKERN_START 	(0)
#define MEMTYPE_LOKERN_END 	((16>>1) < pfndb_max() ? \
				 (16>>1) : pfndb_max())

#define MEMTYPE_KERN_START 	((16>>1) < pfndb_max() ? \
				 (16>>1) : pfndb_max())
#define MEMTYPE_KERN_END 	((768>>1) < pfndb_max() ? \
				 (768>>1) : pfndb_max())

#define MEMTYPE_HIGH_START 	((768>>1) < pfndb_max() ? \
				 (768>>1) : pfndb_max())
#define MEMTYPE_HIGH_END   	(pfndb_max())

/* XXX: UNTIL HERE. */

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
    (PFNZ_KERN(_p) ? PGA_KERN_ONLY :			\
     (PFNZ_LOKERN(_p) ? PGA_LOKERN_ONLY : PGA_HIGH_ONLY))


static struct zone zone_lokern, zone_kern, zone_high;

unsigned
pgalloc(size_t len, unsigned flags, uint8_t type)
{
    unsigned i;
    int ret = 0;

    if (flags == 0)
	flags = PGA_DEFAULT;

    if (flags & PGA_HIGH_ONLY)
	ret = zone_alloc(&zone_high, len);

    if (!ret && (flags & PGA_KERN_ONLY))
	ret = zone_alloc(&zone_kern, len);

    if (!ret && (flags & PGA_LOKERN_ONLY))
	ret = zone_alloc(&zone_kern, len);

    for (i = 0; i < len; i++)
	pfndb_settype(ret + i, type);

    return ret;
}

void
pgfree(unsigned pfn, size_t len)
{
    struct zone *zn = NULL;

    if (PFNZ_KERN(pfn))
	zn = &zone_kern;
    else if (PFNZ_LOKERN(pfn))
	zn = &zone_lokern;
    else if (PFNZ_HIGH(pfn))
	zn = &zone_high;
    assert(zn != NULL);

    zone_free(zn, pfn, len);
}


void
pginit(void)
{
    int status=0; 	/* 1: we're scanning freezone,
			 * 0: we're searching for free zone */
    unsigned start = 0, i;

    zone_init(&zone_high);
    zone_init(&zone_kern);
    zone_init(&zone_lokern);

    for (i = 0; i < pfndb_max(); i++) {
	if ((pfndb_type(i) == PFNT_FREE) && (status == 0)) {
	    status = 1;
	    start = i;
	    continue;
	}
	if ((pfndb_type(i) != PFNT_FREE) && (status == 1)) {
	    status = 0;
	    pgfree(start, i - start);
	    continue;
	}
	/* If we're crossing a memory type boundary, close 
	   the pgzone.  */
	if ((status == 1) &&
	    (pfndb_type(i) == PFNT_FREE) &&
	    (PFNZ_TYPE(i) != PFNZ_TYPE(i-1))) {
	    pgfree(start, i - start);
	    start = i;
	    continue;
	}
    }

    if (status == 1) {
	/* save last freezone */
	pgfree(start, i - start);
    }
}
