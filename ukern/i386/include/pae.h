#ifndef __i386_pae_h
#define __i386_pae_h

#include <uk/types.h>
#include <uk/param.h>

#define KPDPTE 3
#define NPDPTE 4

#define NPTES  512


/* XXX:NOT REALLY PAE: PARAM! */
/* 768mb: Max direct-mapped kernel address */
#define DMAPSIZE   768*1024*1024
#define KERN_SDMAP UKERNBASE
#define KERN_EDMAP UKERNBASE + DMAPSIZE

#define L2SHIFT    30
#define L2MASK     (0x3 << L2SHIFT)
#define L2OFF(_a) (((uintptr_t)(_a) & L2MASK) >> L2SHIFT)
#define L2ADDR(_a) (((_a) & 0x3) << L2SHIFT)

#define L1SHIFT    21
#define L1MASK     (0x1ff << L1SHIFT)
#define L1OFF(_a) (((uintptr_t)(_a) & L1MASK) >> L1SHIFT)
#define L1ADDR(_a) (((_a) & 0x1ff) << L1SHIFT)

/* L0: Actual PAE PTE, used only for linear maps here. Sorry. */
#define L0SHIFT    12
#define L0MASK     (0x1ff << L0SHIFT)
#define L0ADDR(_a) (((_a) & 0x1ff) << L0SHIFT)


#define KL1_SDMAP  L1OFF(KERN_SDMAP)
#define KL1_EDMAP  L1OFF(KERN_EDMAP)
#define NPTES      512

#define PG_P       1
#define PG_W       2
#define PG_U       4
#define PG_A       0x20
#define PG_D       0x40
#define PG_S       0x80
#define PG_NX      0x8000000000000000LL

#define LMAPOFF    (NPTES - 4)

#define PAEOFF2VA(_l2,_l1,_l0) (L2ADDR(_l2) + L1ADDR(_l1) + L0ADDR(_l0))
#define GETL1T(_a) PAEOFF2VA(2, LMAPOFF + 2, LMAPOFF + L2OFF(_a)) 
#define GETL2T(_a) (PAEOFF2VA(2, LMAPOFF + 2, LMAPOFF + 2) + 8 * LMAPOFF)

#define trunc_4k(_a) ((uintptr_t)(_a) & 0xfffff000)
#define mkl1e(_a, _f) (trunc_page((uintptr_t)(_a) - UKERNBASE) | PG_S | (_f))
#define mkl2e(_a, _f) (trunc_4k((uintptr_t)(_a) - UKERNBASE) | (_f))
/* Linear mapping must use 4k mapping. */
#define mklinl1e(_a, _f) (trunc_4k((uintptr_t)(_a) - UKERNBASE) | (_f))

/* We use 2Mb pages, PAE has only 2 levels for to us */
typedef uint64_t l2e_t;
typedef uint64_t l1e_t;

#endif
