#ifndef __i386_vmparam_h
#define __i386_vmparam_h

#include <machine/uk/ukparam.h>

#define PAGE_SIZE   (1 << PAGE_SHIFT)
#define PAGE_MASK   (PAGE_SIZE - 1)

#define trunc_page(x) (((x) >> PAGE_SHIFT) << PAGE_SHIFT)	/* type-safe */
#define round_page(x) trunc_page((x) + PAGE_MASK)

#ifndef _ASSEMBLER

#define atop(x)  ((paddr_t)(x) >> PAGE_SHIFT)
#define ptoa(x)  ((paddr_t)(x) << PAGE_SHIFT)

/* XXX: Not for vmap! */
#define vatop(x) ((vaddr_t)((x) - UKERNBASE) >> PAGE_SHIFT)
#define ptova(x) (UKERNBASE + ((vaddr_t)(x) << PAGE_SHIFT))
#endif



#endif
