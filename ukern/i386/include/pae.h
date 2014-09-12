#ifndef __i386_pae_h
#define __i386_pae_h

#include <uk/types.h>
#include <uk/param.h>

#define KPDPTE 3
#define NPDPTE 4

#define NPTES  512

#define L2SHIFT    30
#define L2MASK     (0x3 << L2SHIFT)
#define L2OFF(_a)  (((uintptr_t)(_a) & L2MASK) >> L2SHIFT)
#define L2VA(_a)   (((_a) & 0x3) << L2SHIFT)

#define L1SHIFT    21
#define L1MASK     (0x1ff << L1SHIFT)
#define L1OFF(_a)  (((uintptr_t)(_a) & L1MASK) >> L1SHIFT)
#define L1VA(_a)   (((_a) & 0x1ff) << L1SHIFT)

/* L0: Actual PAE PTE, used only for linear maps here. Sorry. */
#define L0SHIFT    12
#define L0MASK     (0x1ff << L0SHIFT)
#define L0VA(_a)   (((_a) & 0x1ff) << L0SHIFT)


#define KL1_SDMAP  L1OFF(KVA_SDMAP)
#define KL1_EDMAP  L1OFF(KVA_EDMAP)
#define NPTES      512

#define PG_P       1
#define PG_W       2
#define PG_U       4
#define PG_A       0x20
#define PG_D       0x40
#define PG_S       0x80
#define PG_NX      0x8000000000000000LL

#define LINOFF    (NPTES - 4)

#define __paeoffva(_l2,_l1,_l0) (L2VA(_l2) + L1VA(_l1) + L0VA(_l0))
#define __val1tbl(_va) ((l1e_t *)__paeoffva(2, LINOFF+ 2, LINOFF + L2OFF(_va)))
#define __val2tbl(_va) ((l2e_t *)__paeoffva(2, LINOFF +2, LINOFF + 2) + LINOFF)

#define trunc_4k(_a) ((uintptr_t)(_a) & 0xfffff000)
#define mkl1e(_a, _f) (trunc_page((uintptr_t)(_a)) | PG_S | (_f))
#define mkl2e(_a, _f) (trunc_4k((uintptr_t)(_a)) | (_f))
/* Linear mapping must use 4k mapping. */
#define mklinl1e(_a, _f) (trunc_4k((uintptr_t)(_a)) | (_f))

/* We use 2Mb pages, PAE has only 2 levels for to us */
typedef uint64_t l2e_t;
typedef uint64_t l1e_t;

#endif
