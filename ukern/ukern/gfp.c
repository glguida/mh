/*
 * Copyright (c) 2002, 2014 Gianluca Guida.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * See the README file for information on how to contact the author.
 */

#include <uk/types.h>
#include <uk/assert.h>
#include <uk/locks.h>
#include <ukern/gfp.h>
#include <ukern/pfndb.h>
#include <lib/lib.h>

static lock_t gfplock = 0;
static u_long free_pages;

static __inline uint32_t
lsbit(uint32_t x)
{
    assert (x != 0);
    return ffs(x) - 1;
}

static __inline uint32_t
msbit(uint32_t x)
{
    assert (x != 0);
    return fls(x) - 1;
}

/* O(1) Zoned Page Allocator */

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
    (PFNZ_KERN(_p) ? GFP_KERN_ONLY :			\
     (PFNZ_LOKERN(_p) ? GFP_LOKERN_ONLY : GFP_HIGH_ONLY))

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


/* if you change this, be sure to adapt the bitmap size */
#define GFP_MAXORDER 32

static uint32_t zones_bitmap_high;
static uint32_t zones_bitmap_kern;
static uint32_t zones_bitmap_lokern;

LIST_HEAD(pgzlist, pgzone); /* struct defined. */
static struct pgzlist zones_shortcut_high[GFP_MAXORDER];
static struct pgzlist zones_shortcut_kern[GFP_MAXORDER];
static struct pgzlist zones_shortcut_lokern[GFP_MAXORDER];


static int
pgzone_detachzone(struct pgzlist *zlist, uint32_t *bmap, struct pgzone *pz)
{
    u_long msb;

    assert(pz->size != 0);
    msb = msbit(pz->size);
    assert(msb >= GFP_MAXORDER);

    LIST_REMOVE(pz, list);
    if (LIST_EMPTY(&zlist[msb]))
	*bmap &= ~(1UL << msb);
    free_pages -= pz->size;
  
    return 1;
}


static int
pgzone_attachzone(struct pgzlist *zlist, uint32_t *bmap, struct pgzone *pz)
{
    u_long msb;

    assert(pz->size != 0);
    msb = msbit(pz->size);
    assert(msb >= GFP_MAXORDER);

    *bmap |= (1UL << msb);
    LIST_INSERT_HEAD(&zlist[msb], pz, list);
    free_pages += pz->size;
    return 1;
}

static struct pgzone *
pgzone_makeptr(u_long ppn, u_long size)
{
    int i;
    struct pgzone *pz1 = (struct pgzone *)pfndb_getptr(ppn);
    struct pgzone *pz2;

    pz1->ppn = ppn;
    pz1->size = size;

    if (size==1) {
	pfndb_settype(ppn, PFNT_FREE_PZ_LONE);
	return pz1;
    }

    pz2 = (struct pgzone *)pfndb_getptr(ppn + size - 1);
    pz2->ppn = ppn;
    pz2->size = size;

    pfndb_settype(ppn, PFNT_FREE_PZ_STRT);
    for (i = 1; i < size - 2; i++) {
	pfndb_settype(ppn + i, PFNT_FREE);
    }
    pfndb_settype(ppn + size - 1, PFNT_FREE_PZ_TERM);  
    return pz1;
}


static struct pgzone *
findfreepgzone(u_long n, u_long flag)
{
    uint32_t tmp;
    unsigned int minbit;
    struct pgzone *pz = NULL;

    minbit = msbit(n);

    if (n != (1 << minbit))
	minbit += 1;

    if (minbit >= GFP_MAXORDER) {
	/* Wrong size */
	return NULL;
    }

    if (flag & GFP_HIGH_ONLY) {
	tmp = zones_bitmap_high >> minbit;
	if (tmp) {
	    tmp = lsbit(tmp);
	    pz = LIST_FIRST(&zones_shortcut_high[minbit + tmp]);
	    goto out;
	}
    }

    if (flag & GFP_KERN_ONLY) {
	tmp = zones_bitmap_kern >> minbit;

	if (tmp) {
	    tmp = lsbit(tmp);
	    pz = LIST_FIRST(&zones_shortcut_kern[minbit + tmp]);
	    goto out;
	}
    }

    if (flag & GFP_LOKERN_ONLY) {
	tmp = zones_bitmap_lokern >> minbit;
	if (tmp) {
	    tmp = lsbit(tmp);
	    pz = LIST_FIRST(&zones_shortcut_lokern[minbit + tmp]);
	    goto out;
	}
    }

  out:
    return pz;
}

static int
pgzone_remove(u_long ppn)
{
    struct pgzone *pz;

    /* XXX: check that mmap[ppn].pz is a start of a zone and 
       return the pointer */
    if (!is_pgzstart(ppn))
	return -1;

    pz = (struct pgzone *)pfndb_getptr(ppn);
    if (pz==NULL)
	return -1;

    if (PFNZ_LOKERN(pz->ppn))
	pgzone_detachzone(zones_shortcut_lokern,
			    &zones_bitmap_lokern, pz);
    else if (PFNZ_KERN(pz->ppn))
	pgzone_detachzone(zones_shortcut_kern,
			    &zones_bitmap_kern, pz);
    else if (PFNZ_HIGH(pz->ppn))
	pgzone_detachzone(zones_shortcut_high,
			    &zones_bitmap_high, pz);
    return 1;
}


static int
pgzone_checkbtmjoin(u_long ppn)
{

    /* Can't go lower than zero. */
    if (ppn == 0) {
	return 0;
    }
  
    /* 
     * We're crossing a memory zone (LOKERN, HIGH, KERN) boundary,
     * do not join.
     */
    if (PFNZ_TYPE(ppn) != PFNZ_TYPE(ppn - 1))
	return 0;

    if (is_pgzfree(ppn - 1)) {
	assert(is_pgzterm(ppn));
	return 1;
    }

    return 0;
}


static int
pgzone_checkupjoin(ppn, size)
{

    if (ppn+size >= pfndb_max()) {
	return 0;
    }
  
    /* 
     * We're crossing a memory zone (LOKERN, HIGH, KERN) boundary,
     * do not join.
     */
    if (PFNZ_TYPE(ppn) != PFNZ_TYPE(ppn+size))
	return 0;

    if (is_pgzfree(ppn + size)) {
	assert(is_pgzstart(ppn + size));
	return 1;
    }

    return 0;
}


static struct pgzone *
pgzone_btmjoin(struct pgzone *pz)
{
    struct pgzone *pzbtm = (struct pgzone *)pfndb_getptr(pz->ppn - 1);
    u_long ppn, sz;
  
    ppn = pzbtm->ppn;
    sz = pzbtm->size + pz->size;
    pgzone_remove(ppn);
  
    return pgzone_makeptr(ppn, sz);
}


static struct pgzone *
pgzone_upjoin(struct pgzone *pz)
{
    struct pgzone *pzup = (struct pgzone *)pfndb_getptr(pz->ppn - 1);
    u_long sz;
  
    sz = pz->size + pzup->size;
    pgzone_remove(pz->ppn + pz->size);
  
    return pgzone_makeptr(pz->ppn, sz);
}


static int
pgzone_create(u_long ppn, u_long size)
{
    struct pgzone *pz=NULL;
  
    pz = pgzone_makeptr(ppn, size);
  
    if (pgzone_checkbtmjoin(pz->ppn))
	pz = pgzone_btmjoin(pz);
    if (pgzone_checkupjoin(pz->ppn, pz->size))
	pz = pgzone_upjoin(pz);


    if (PFNZ_LOKERN(pz->ppn))
	pgzone_attachzone(zones_shortcut_lokern,
			  &zones_bitmap_lokern, pz);
    else if (PFNZ_KERN(pz->ppn))
	pgzone_attachzone(zones_shortcut_kern,
			  &zones_bitmap_kern, pz);
    else if (PFNZ_HIGH(pz->ppn))
	pgzone_attachzone(zones_shortcut_high,
			  &zones_bitmap_high, pz);
    else
	return 0;
    return 1;
}


void freepages(u_long ppn, u_long n)
{

    assert(ppn + n < pfndb_max());
    assert(PFNZ_TYPE(ppn) == PFNZ_TYPE(ppn+n-1));

    spinlock(&gfplock);
    pgzone_create(ppn, n);
    spinunlock(&gfplock);
}

long getfreepages(u_long n, u_long type, u_long flags)
{
    int i;
    struct pgzone *pz;

    assert (n != 0);
    assert(!is_pgzfree(type));

    if (flags == 0)
	flags = GFP_DEFAULT;

    spinlock(&gfplock);

    pz = findfreepgzone(n, flags);
    if (pz == NULL) {
	spinunlock(&gfplock);
	panic("Memory exhausted.\n");
    }
    assert (pz->size >= n);
 
    pgzone_remove(pz->ppn);
    for (i = 0; i < n; i++)
	pfndb_settype(pz->ppn + i, type);

    if ((pz->size - n) > 0)
	pgzone_create(pz->ppn + n, pz->size - n);

    spinunlock(&gfplock);
 
    return pz->ppn;
}

void getfreepages_init(void)
{
    u_long start, i;
    int status=0; 	/* 1: we're scanning freezone,
			 * 0: we're searching for free zone */
  
    free_pages = 0;
  
    for (i = 0; i < GFP_MAXORDER; i++) {
	LIST_INIT(&zones_shortcut_lokern[i]);
	LIST_INIT(&zones_shortcut_kern[i]);
	LIST_INIT(&zones_shortcut_high[i]);
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
	    pgzone_create(start, i - start);
	    continue;
	}
	/* If we're crossing a memory type boundary, close 
	   the pgzone.  */
	if ((status == 1) &&
	    (pfndb_type(i) == PFNT_FREE) &&
	    (PFNZ_TYPE(i) != PFNZ_TYPE(i-1))) {
	    pgzone_create(start, i - start);
	    start = i;
	    continue;
	}
    }

    if (status == 1) {
	/* save last freezone */
	pgzone_create(start, i - start);
    }
  
    printf("Free pages: %ld\n", free_pages);
}
