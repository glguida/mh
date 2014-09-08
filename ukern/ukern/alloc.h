#ifndef __alloc_h
#define __alloc_h

#include <uk/queue.h>

/* if you change this, be sure to adapt the bitmap size */
#define ORDMAX 32

struct zentry {
    LIST_ENTRY(zentry) list;
    uintptr_t addr;
    size_t   len;
};

LIST_HEAD(zentries, zentry); /* typedef */
struct zone {
    lock_t lock;
    uint32_t bmap;
    struct zentries zentries[ORDMAX];
    u_long nfree;
};

#endif
