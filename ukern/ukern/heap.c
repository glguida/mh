#include <uk/types.h>
#include <uk/param.h>
#include <uk/assert.h>
#include <ukern/pfndb.h>
#include <lib/lib.h>

#define LIST_PREV(elm, type, field) ((elm)->field.le_prev ?		\
			      (struct type *)((char *)(elm)->field.le_prev - \
					      offsetof(struct type, field)) \
			      : NULL)

struct ekheap {
    int isfree;
    size_t ulen;
    LIST_ENTRY(ekheap) list;
    LIST_ENTRY(ekheap) free_list;
};

struct kheap {
    LIST_ENTRY(kheap) kheaps;
    LIST_HEAD(, ekheap) list;
    LIST_HEAD(, ekheap) free_list;
};

LIST_HEAD(, kheap) kheaps;

#define UNITS(_s) ((_s) / sizeof(struct ekheap))
#define UROUND(_s) (UNITS((_s) + sizeof(struct ekheap) - 1))
#define HEAP_MAXALLOC (UNITS(PAGE_SIZE) - 1)

static struct kheap *
kheap_add(void)
{
    pfn_t pfn = __allocpage(PFNT_HEAP);
    struct kheap *hptr;
    struct ekheap *eptr;

    /* Lock free, not connected */
    hptr = pfndb_getptr(pfn);
    LIST_INIT(&hptr->list);
    LIST_INIT(&hptr->free_list);
    
    eptr = (struct ekheap *)ptova(pfn);
    eptr->isfree = 1;
    eptr->ulen = UNITS(PAGE_SIZE); 
    LIST_INSERT_HEAD(&hptr->list, eptr, list);
    LIST_INSERT_HEAD(&hptr->free_list, eptr, free_list);
    return hptr;
}

static void *
kheap_alloc(struct kheap *hptr, size_t sz)
{
    struct ekheap *eptr, *neptr;
    unsigned udiff;
    unsigned units = UROUND(sz) + 1; /* Account for hdr */

    LIST_FOREACH(eptr, &hptr->free_list, free_list)
	if (eptr->ulen >= units)
	    break;

    if (eptr == LIST_END(&hptr->free_list))
	return NULL;

    assert(eptr->isfree);
    udiff = eptr->ulen - units;
    if (udiff) {
	neptr = eptr + units;
	neptr->isfree = 1;
	neptr->ulen = udiff;
	/* LOCK */
	LIST_INSERT_AFTER(eptr, neptr, list);
	LIST_INSERT_HEAD(&hptr->free_list, neptr, free_list);
    }

    /* LOCK */
    LIST_REMOVE(eptr, free_list);
    eptr->isfree = 0;
    eptr->ulen = units;

    return eptr + 1;
}

static void
kheap_free(void *ptr)
{
    unsigned pfn = vatop(ptr);
    struct kheap *hptr;
    struct ekheap *eptr, *peptr, *neptr;

    assert(pfndb_type(pfn) == PFNT_HEAP);
    hptr = (struct kheap *)pfndb_getptr(pfn);
    eptr = (struct ekheap *)ptr - 1;

    /* LOCK */
    peptr = LIST_PREV(eptr, ekheap, list);
    neptr = LIST_NEXT(eptr, list);
    if (neptr && neptr->isfree) {
	neptr->isfree = 0;
	LIST_REMOVE(neptr, free_list);
	LIST_REMOVE(neptr, list);
	eptr->ulen += neptr->ulen;
    }
    if (peptr && peptr->isfree) {
	LIST_REMOVE(eptr, list);
	peptr->ulen += eptr->ulen;
    } else {
	eptr->isfree=1;
	LIST_INSERT_HEAD(&hptr->free_list, eptr, free_list);
    }
}

void *
heap_alloc(size_t size)
{
    void *ptr = NULL;
    struct kheap *hptr;

    if (size >= HEAP_MAXALLOC) {
	panic("heap: size too big");
	return NULL;
    }

    /* LOCK */
    LIST_FOREACH(hptr, &kheaps, kheaps)
      if ((ptr = kheap_alloc(hptr, size)) != NULL)
	    break;
    if (ptr != NULL)
	return ptr;

    hptr = kheap_add();
    ptr = kheap_alloc(hptr, size);
    /* LOCK */
    LIST_INSERT_HEAD(&kheaps, hptr, kheaps);
    return ptr;
}

void
heap_free(void *ptr)
{
    kheap_free(ptr);
}

void heap_init(void)
{
    LIST_INIT(&kheaps);
}

void
heap_shrink(void)
{

#warning implement me
}
