#include <uk/cdefs.h>
#include <uk/types.h>
#include <uk/assert.h>
#include <uk/queue.h>
#include <lib/lib.h>

#ifndef ALLOCFUNC
#define ALLOCFUNC(...) zone_##__VA_ARGS__
#endif

#ifdef ALLOCDEBUG
#define dbgprintf(...) printf(__VA_ARGS__)
#else
#define dbgprintf(...) do { /* nothing */ }while(0)
#endif

#ifndef __ZADDR_T
#error __ZADDR_T not defined
#else
typedef __ZADDR_T zaddr_t;
#endif

/* Basic zentry structure must contain these fields:

   struct zentry {
       LIST_ENTRY(zentry) list;
       zaddr_t addr;
       size_t size;
   };

*/

static __inline uint32_t lsbit(uint32_t x)
{
	assert(x != 0);
	return ffs(x) - 1;
}

static __inline uint32_t msbit(uint32_t x)
{
	assert(x != 0);
	return fls(x) - 1;
}

#define ORDMAX 32

struct __ZENTRY;
LIST_HEAD(zlist, __ZENTRY);
struct zone {
	uint32_t bmap;
	struct zlist zlist[ORDMAX];
	unsigned nfree;
};

static void zone_detachentry(struct zone *z, struct __ZENTRY *ze)
{
	uint32_t msb;

	assert(ze->size != 0);
	msb = msbit(ze->size);
	assert(msb < ORDMAX);

	LIST_REMOVE(ze, list);
	dbgprintf("LIST_REMOVE: %p (%lx ->", ze, z->bmap);
	if (LIST_EMPTY(z->zlist + msb))
		z->bmap &= ~(1UL << msb);
	dbgprintf(" %lx)\n", z->bmap);
	z->nfree -= ze->size;
	dbgprintf("D<%p>(%lx,%lx)", ze, ze->addr, ze->size);
}

static void zone_attachentry(struct zone *z, struct __ZENTRY *ze)
{
	uint32_t msb;

	assert(ze->size != 0);
	msb = msbit(ze->size);
	assert(msb < ORDMAX);

	dbgprintf("LIST_INSERT(%p + %d, %p), bmap (%lx ->", z->zlist, msb,
		  ze, z->bmap);
	z->bmap |= (1UL << msb);
	dbgprintf(" %lx\n", z->bmap);


	LIST_INSERT_HEAD(z->zlist + msb, ze, list);
	z->nfree += ze->size;
	dbgprintf("A<%p>(%lx,%lx)", ze, ze->addr, ze->size);
}

static struct __ZENTRY *zone_findfree(struct zone *zn, size_t size)
{
	uint32_t tmp;
	unsigned int minbit;
	struct __ZENTRY *ze = NULL;

	minbit = msbit(size);

	if (size != (1 << minbit))
		minbit += 1;

	if (minbit >= ORDMAX) {
		/* Wrong size */
		return NULL;
	}

	tmp = zn->bmap >> minbit;
	if (tmp) {
		tmp = lsbit(tmp);
		ze = LIST_FIRST(zn->zlist + minbit + tmp);
		dbgprintf("LIST_FIRST(%p + %d + %d) = %p\n", zn->zlist,
			  minbit, tmp, ze);
	}
	dbgprintf("FFREE(%p):(%lx,%lx)\n", ze, ze->addr, ze->size);
	return ze;
}

static void zone_remove(struct zone *z, struct __ZENTRY *ze)
{
	zone_detachentry(z, ze);
	___freeptr(ze);
}

static void zone_create(struct zone *z, zaddr_t zaddr, size_t size)
{
	struct __ZENTRY *ze, *pze = NULL, *nze = NULL;
	zaddr_t fprev = zaddr, lnext = zaddr + size;
	dbgprintf("HHH");
	___get_neighbors(zaddr, size, &pze, &nze);
	dbgprintf("HHH");
	if (pze) {
		fprev = pze->addr;
		zone_remove(z, pze);
	}
	dbgprintf("HHH");
	if (nze) {
		lnext = nze->addr + nze->size;
		zone_remove(z, nze);
	}
	dbgprintf("HHH");
	ze = ___mkptr(fprev, lnext - fprev);
	dbgprintf("MKPTR(%p): %lx,%lx\n", ze, ze->addr, ze->size);
	zone_attachentry(z, ze);
}


void ALLOCFUNC(free) (struct zone * z, zaddr_t zaddr, size_t size) {

	assert(size != 0);
	dbgprintf("Freeing %lx\n", zaddr);
	zone_create(z, zaddr, size);
}

zaddr_t ALLOCFUNC(alloc) (struct zone * z, size_t size) {
	struct __ZENTRY *ze;
	zaddr_t addr = 0;
	long diff;

	assert(size != 0);

	ze = zone_findfree(z, size);
	if (ze == NULL)
		goto out;

	addr = ze->addr;
	diff = ze->size - size;
	assert(diff >= 0);
	zone_remove(z, ze);
	if (diff > 0)
		zone_create(z, addr + size, diff);

      out:
	dbgprintf("Allocating %lx\n", addr);
	return addr;
}

void ALLOCFUNC(init) (struct zone * z) {
	int i;

	z->bmap = 0;
	z->nfree = 0;
	for (i = 0; i < ORDMAX; i++)
		LIST_INIT(z->zlist + i);
}
