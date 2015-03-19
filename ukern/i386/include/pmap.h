#ifndef __i386_pmap_h
#define __i386_pmap_h

#include <uk/queue.h>
#include <uk/param.h>
#include <machine/uk/pae.h>

struct pmap {
	/* Needs to be first and cache aligned (32-byte needed by HW) */
	l2e_t pdptr[NPDPTE];
	l1e_t *l1s;

	unsigned tlbflush;
	cpumask_t cpumap;
	unsigned refcnt;
	lock_t lock;
};

struct pv_entry {
	LIST_ENTRY(pv_entry) list_entry;
	struct pmap *pmap;
	unsigned vfn;
};

void pmap_init(void);
struct pmap *pmap_boot(void);
struct pmap *pmap_alloc(void);
void pmap_switch(struct pmap *pmap);

#define PROT_KERNWRX   (PROT_KERNX | PG_A | PG_D | PG_W | PG_D)
#define PROT_KERNWR    (PROT_KERN | PG_A | PG_D | PG_W | PG_D)
#define PROT_KERN      (PG_P | PG_NX)
#define PROT_KERNX     PG_P
#define PROT_GLOBAL    PG_G
#define PROT_USER      (PG_U | PG_NX | PG_P)
#define PROT_USER_WR   (PG_W | PG_U | PG_NX | PG_P)
#define PROT_USER_WRX  (PG_W | PG_U | PG_P)
typedef unsigned pmap_prot_t;

#define FAULT_P 1
#define FAULT_W 2
#define FAULT_U 4
#define FAULT_X 8
typedef unsigned pmap_fault_t;

l1e_t pmap_setl1e(struct pmap *pmap, vaddr_t va, l1e_t l1e);
pfn_t pmap_enter(struct pmap *pmap, vaddr_t va, paddr_t pa,
		 unsigned flags);
void pmap_commit(struct pmap *pmap);

//#define pmap_enter(_pmap, _va, _pa, _prot) atop(pmap_setl1e((_pmap), (_va), mkl1e((_pa), (_prot))))
#define pmap_clear(_pmap, _va) pmap_setl1e((_pmap), (_va), 0)
#define pmap_current()                                          \
    ((struct pmap *)((uintptr_t)UKERNBASE + __getpdptr()        \
                     - offsetof(struct pmap, pdptr)))

#endif
