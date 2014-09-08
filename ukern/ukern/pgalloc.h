#ifndef __pgalloc_h
#define __pgalloc_h

#include "alloc.h"

void pginit(void);
void pgfree(unsigned pfn, size_t len);
unsigned pgalloc(size_t len, unsigned flags, uint8_t type);

#define PGA_LOKERN_ONLY 1
#define PGA_KERN_ONLY   2
#define PGA_HIGH_ONLY   4

#define PGA_LOKERN PGA_LOKERN_ONLY
#define PGA_KERN (PGA_LOKERN_ONLY | PGA_KERN_ONLY)
#define PGA_HIGH (PGA_HIGH_ONLY | PGA_KERN)
#define PGA_DEFAULT PGA_KERN

#define __allocpage(_t) pgalloc(1, PGA_KERN, (_t))
#define __freepage(_p) pgfree((_p), 1);

#endif
