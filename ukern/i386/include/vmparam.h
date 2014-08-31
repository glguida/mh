#ifndef __i386_vmparam_h
#define __i386_vmparam_h

#define PAGE_SHIFT   21

#define PAGE_SIZE   (1 << PAGE_SHIFT)
#define PAGE_MASK   (PAGE_SIZE - 1)

#define trunc_page(x) ((addr_t)(x) & ~PAGE_MASK)
#define round_page(x) trunc_page((x) + PAGE_MASK)

#define atop(x) ((addr_t)(x) >> PAGE_SHIFT)
#define ptoa(x) ((addr_t)(x) << PAGE_SHIFT)


#endif
