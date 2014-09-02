struct slab;
struct objhdr;

/*
 * Implementation-dependent code.
 */

#include <uk/cdefs.h>
#include <uk/param.h>
#include <uk/types.h>
#include <uk/queue.h>
#include <uk/locks.h>
#include <ukern/pfndb.h>
#include <ukern/gfp.h>
#include <lib/lib.h>

#define DECLARE_SPIN_LOCK(_x) lock_t _x
#define SPIN_LOCK_INIT(_x) do { _x = 0; } while (0)
#define SPIN_LOCK(_x) spinlock(&_x)
#define SPIN_UNLOCK(_x) spinunlock(&_x)
#define SPIN_LOCK_FREE(_x) 

#ifdef __SLAB_FIXMEM

#define SLABFUNC(_s) fixmem##_s
#define SLABSQUEUE fixmemq

#include <ukern/fixmem.h>

#define ___slabsize() PAGE_SIZE

static const size_t
___slabobjs(const size_t size)
{

    return ___slabsize()/size;
}

static struct slabhdr *
___slaballoc(struct objhdr **ohptr)
{
    long pfn;

    pfn = getfreepage(PFNT_FIXMEM);
    if (pfn < 0)
	return NULL;

    *ohptr = (struct objhdr *)ptova(pfn);
    return pfndb_slabh(pfn);
}

static struct slabhdr *
___slabgethdr(void *obj)
{
    unsigned pfn;
    uintptr_t addr = (uintptr_t)obj;

    pfn = trunc_page(addr);
    return pfndb_slabh(pfn);
}

static void
___slabfree(void *addr)
{
    long pfn;

    pfn = vatop(addr);
    freepage(pfn);
}


#elif __SLAB_STRUCTALLOC

static const size_t
___slabobjs(const size_t size)
{

    return (___slabsize() - sizeof(struct slabhdr))/size;
}

static struct slabhdr *
___slab_alloc(struct objhdr **ptroh);
{
    uintptr_t addr;

    addr = mach_getpage()
	return NULL;

    *oh = (struct objhdr *)(addr + sizeof(slabhdr))
    return (struct slabhdr *)addr;
}

static struct slabhdr *
___slabgethdr(void *obj)
{
     struct slabhdr *sh;
     uintptr_t addr = (uintptr_t)obj;

     sh = (struct slabhdr *)(addr & ~((uintptr_t)slab_size -1));
     if (sh->magic != slab_magic)
	  return NULL;
     return sh;
}

#endif


/*
 * Generic, simple and portable slab allocator.
 */

#include "slab.h"

#define MAGIC_SIZE 16
#define MAGIC 0xdead5ade

static int initialised = 0;
static size_t slab_size = 0;
static const uint64_t slab_magic = MAGIC;
static LIST_HEAD(slabqueue, slab) SLABSQUEUE;
static unsigned slabs = 0;
DECLARE_SPIN_LOCK(slabs_lock);

struct objhdr
{
     SLIST_ENTRY(objhdr) list_entry;
};

struct slabhdr
{
     union {
     struct {
	  uint64_t             magic;
	  struct slab          *cache;
	  unsigned             freecnt;
	  SLIST_HEAD(, objhdr) freeq;
	  
	  LIST_ENTRY(slabhdr)  list_entry;
     };
     char cacheline[64];
     };

};

int
SLABFUNC(_grow)(struct slab *sc)
{
     int i;
     struct objhdr *ptr;
     struct slabhdr *sh;
     const size_t objs = ___slabobjs(sc->objsize);

     sh = ___slaballoc(&ptr);
     if (sh == NULL)
	 panic("OOM");

     sh->magic = slab_magic;
     sh->cache = sc;
     sh->freecnt = objs;
     SLIST_INIT(&sh->freeq);

     for (i = 0; i < objs; i++) {
          SLIST_INSERT_HEAD(&sh->freeq, ptr, list_entry);
	  ptr = (struct objhdr *)((uint8_t *)ptr + sc->objsize);
     }

     SPIN_LOCK(sc->lock);
     LIST_INSERT_HEAD(&sc->emptyq, sh, list_entry);
     sc->emptycnt++;
     SPIN_UNLOCK(sc->lock);

     return 1;
}

int
SLABFUNC(_shrink)(struct slab *sc)
{
     int shrunk = 0;
     struct slabhdr *sh;

     SPIN_LOCK(sc->lock);
     while (!LIST_EMPTY(&sc->emptyq)) {
	  sh = LIST_FIRST(&sc->emptyq);
	  LIST_REMOVE(sh, list_entry);
	  sc->emptycnt--;

	  SPIN_UNLOCK(sc->lock);
	  ___slabfree((void *)sh);
	  shrunk++;
	  SPIN_LOCK(sc->lock);
     }
     SPIN_UNLOCK(sc->lock);

     return shrunk;
}

void *
SLABFUNC(_alloc)(struct slab *sc, void *opq)
{
     int tries = 0;
     void *addr = NULL;
     struct objhdr *oh;
     struct slabhdr *sh = NULL;


     SPIN_LOCK(sc->lock);

retry:
     if (!LIST_EMPTY(&sc->freeq)) {
	  sh = LIST_FIRST(&sc->freeq);
	  if (sh->freecnt == 1) {
	       LIST_REMOVE(sh, list_entry);
	       LIST_INSERT_HEAD(&sc->fullq, sh, list_entry);
	       sc->freecnt--;
	       sc->fullcnt++;
	  }
     }

     if (!sh && !LIST_EMPTY(&sc->emptyq)) {
	  sh = LIST_FIRST(&sc->emptyq);
	  LIST_REMOVE(sh, list_entry);
	  LIST_INSERT_HEAD(&sc->freeq, sh, list_entry);
	  sc->emptycnt--;
	  sc->freecnt++;
     }

     if (!sh) {
	  SPIN_UNLOCK(sc->lock);

	  if (tries++ > 3)
	       goto out;

	  SLABFUNC(_grow)(sc);

	  SPIN_LOCK(sc->lock);
	  goto retry;
     }

     oh = SLIST_FIRST(&sh->freeq);
     SLIST_REMOVE_HEAD(&sh->freeq, list_entry);
     sh->freecnt--;

     SPIN_UNLOCK(sc->lock);

     addr = (void *)oh;
     memset(addr, 0, sizeof(*oh));

     if (sc->ctr)
	  sc->ctr(addr, opq, 0);

 out:
     return addr;
}

void
SLABFUNC(_free)(void *ptr)
{
struct slab *sc;
     struct slabhdr *sh;
     unsigned max_objs;

     sh = ___slabgethdr(ptr);
     if (!sh)
	 return;
     sc = sh->cache;
     max_objs = (slab_size - sizeof(struct slabhdr)) / sc->objsize;

     if (sc->ctr)
	 sc->ctr(ptr, NULL, 1);

     SPIN_LOCK(sc->lock);
     SLIST_INSERT_HEAD(&sh->freeq, (struct objhdr *)ptr, list_entry);
     sh->freecnt++;

     if (sh->freecnt == 1) {
	  LIST_REMOVE(sh, list_entry);
	  LIST_INSERT_HEAD(&sc->freeq, sh, list_entry);
	  sc->fullcnt--;
	  sc->freecnt++;
     } else if (sh->freecnt == max_objs) {
	  LIST_REMOVE(sh, list_entry);
	  LIST_INSERT_HEAD(&sc->emptyq, sh, list_entry);
	  sc->freecnt--;
	  sc->emptycnt++;
     }
     SPIN_UNLOCK(sc->lock);

     return;
}

int
SLABFUNC(_register)(struct slab *sc, char *name, size_t objsize,
		    void (*ctr)(void *, void *, int),	int cachealign)
{

    if (initialised == 0) {
	slab_size = ___slabsize();
	LIST_INIT(&SLABSQUEUE);
	SPIN_LOCK_INIT(slabs_lock);
	initialised++;
    }

    SPIN_LOCK_INIT(sc->lock);
    sc->name = name;

    if (cachealign) {
	sc->objsize = ((objsize + 63) & ~63L);
    } else {
	sc->objsize = objsize;
    }
    sc->ctr = ctr;

    sc->emptycnt = 0;
    sc->freecnt = 0;
    sc->fullcnt = 0;

    LIST_INIT(&sc->emptyq);
    LIST_INIT(&sc->freeq);
    LIST_INIT(&sc->fullq);

    SPIN_LOCK(slabs_lock);
    LIST_INSERT_HEAD(&SLABSQUEUE, sc, list_entry);
    slabs++;
    SPIN_UNLOCK(slabs_lock);

    return 0;
}

void
SLABFUNC(_deregister)(struct slab *sc)
{
     struct slabhdr *sh;

     while (!LIST_EMPTY(&sc->emptyq)) {
	  sh = LIST_FIRST(&sc->emptyq);
	  LIST_REMOVE(sh, list_entry);

	  ___slabfree((void *)sh);
     }

     while (!LIST_EMPTY(&sc->freeq)) {
	  sh = LIST_FIRST(&sc->freeq);
	  LIST_REMOVE(sh, list_entry);
	  ___slabfree((void *)sh);
     }

     while (!LIST_EMPTY(&sc->fullq)) {
	  sh = LIST_FIRST(&sc->fullq);
	  LIST_REMOVE(sh, list_entry);
	  ___slabfree((void *)sh);
     }

     SPIN_LOCK_FREE(sc->lock);

     SPIN_LOCK(slabs_lock);
     LIST_REMOVE(sc, list_entry);
     slabs--;
     SPIN_UNLOCK(slabs_lock);
}
