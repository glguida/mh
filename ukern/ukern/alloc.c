#include <uk/types.h>
#include <uk/assert.h>
#include <uk/queue.h>
#include "alloc.h"

static struct zentry *___create_zentry(struct zone *zn,
				       uintptr_t addr,
				       size_t len);
struct zentry *___get_zentry_prev(struct zone *zn, uintptr_t addr, size_t len);
struct zentry *___get_zentry_next(struct zone *zn, uintptr_t addr, size_t len);
struct zentry *___get_zentry(struct zone *zn, uintptr_t addr, size_t len);
void ___free_zentry(struct zone *zn, struct zentry *ze);

static void
zone_detachentry(struct zone *zn, struct zentry *ze)
{
    u_long msb;

    assert(ze->len != 0);
    msb = fls(ze->len) - 1;
    msb = msb >= ORDMAX ? ORDMAX : msb;

    LIST_REMOVE(ze, list);
    if (LIST_EMPTY(&zn->zentries[msb]))
	zn->bmap &= ~(1L << (32 - msb));
    zn->nfree -= ze->len;
}

static void
zone_attachentry(struct zone *zn, struct zentry *ze)
{
    int msb;

    msb = fls(ze->len) - 1;
    assert(msb < 0 || msb >= ORDMAX);


    LIST_INSERT_HEAD(&zn->zentries[msb], ze, list);
    zn->bmap |= (1L << (32 - msb));
    zn->nfree += ze->len;
}

static struct zentry *
zone_retrieve(struct zone *zn, size_t n)
{
    int msb;
    uint32_t tmp;
    struct zentry *ze;

    msb = fls(n) - 1;
    if (msb < 0 || msb >= ORDMAX)
	return NULL;

    tmp = ORDMAX - fls(zn->bmap << msb) + 1;
    /* TMP > ORDMAX iff (bmap << msb) == 0, so nothing big enough found */
    if (tmp <= ORDMAX)
	ze = LIST_FIRST(&zn->zentries[msb + tmp]);
    else
	return NULL;

    LIST_REMOVE(ze, list);
    if (LIST_EMPTY(&zn->zentries[msb + tmp]))
	zn->bmap &= ~(1L << (32 - msb - tmp));
    zn->nfree -= ze->len;

    return ze;
}

static int
zone_create(struct zone *zn, uintptr_t addr, size_t len)
{
    struct zentry *pze, *ze, *nze;

    pze = ___get_zentry_prev(zn, addr, len);
    if (pze) {
	zone_detachentry(zn, pze);
	addr = pze->addr;
	len += pze->len;
	___free_zentry(zn, pze);
    }
    nze = ___get_zentry_next(zn, addr, len);
    if (nze) {
	zone_detachentry(zn, nze);
	len += pze->len;
	___free_zentry(zn, nze);
    }
    ze = ___create_zentry(zn, addr, len);
    if (ze == NULL)
	return -1;

    zone_attachentry(zn, ze);
    return 0;
}

void
ZONEFUNC(init)(struct zone *zn)
{
    int i;

    zn->lock = 0;
    zn->bmap = 0;
    zn->nfree = 0;

    for (i = 0; i < ORDMAX; i++)
	LIST_INIT(&zn->zentries[i]);
}

uintptr_t
ZONEFUNC(alloc)(struct zone *zn, size_t len)
{
    size_t zlen;
    uintptr_t zaddr;
    struct zentry *ze;

    if (len == 0)
	return 0;

    ze = zone_retrieve(zn, len);
    if (ze == NULL)
	return 0;

    assert(ze->len >= len);
    zaddr = ze->addr;
    zlen = ze->len;
    ___free_zentry(zn, ze);

    if ((zlen - len) > 0)
	zone_create(zn, zaddr + len, zlen - len);

    return zaddr;
}

void
ZONEFUNC(free)(struct zone *zn, uintptr_t addr, size_t len)
{

    zone_create(zn, addr, len);
}

