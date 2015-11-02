#ifndef __ukparam_h
#define __ukparam_h

#define PAGE_SHIFT   21

#define LPDPTE 2 /* Linear map L2 PTE. */
#define KPDPTE 3 /* Kernel L2 PTE. */
#define NPDPTE 4

#define NPTES 512
#define LINOFF (NPTES - 4)

#define L2_SHIFT 30
#define L1_SHIFT 21

#endif
