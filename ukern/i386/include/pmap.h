#ifndef __i386_pmap_h
#define __i386_pmap_h

#include <uk/queue.h>
#include <uk/param.h>
#include <machine/uk/pae.h>

struct pmap {
    /* Needs to be first and cache aligned (32-byte needed by HW) */
    l2e_t  pdptr[NPDPTE];    

    unsigned refcnt;
    lock_t   lock;
};

struct pv_entry {
    LIST_ENTRY(pv_entry) list_entry;
    struct pmap *pmap;
    unsigned    vfn;
};

struct pmap *pmap_boot(void);
struct pmap *pmap_alloc(void);

#endif
