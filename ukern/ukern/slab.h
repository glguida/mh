#ifndef __slab_h
#define __slab_h

#include <uk/queue.h>

struct slab
{
    lock_t lock;
    char *name;
    size_t objsize;
    void (*ctr)(void *obj, void *opq, int dec);
    unsigned emptycnt;
    unsigned freecnt;
    unsigned fullcnt;
    LIST_HEAD(, slabhdr) emptyq;
    LIST_HEAD(, slabhdr) freeq;
    LIST_HEAD(, slabhdr) fullq;

    LIST_ENTRY(slab) list_entry;
};

#endif
