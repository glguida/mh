#ifndef __ukern_pgalloc_h
#define __ukern_pgalloc_h

#include <uk/types.h>
#include <uk/queue.h>

struct pgzentry {
    LIST_ENTRY(pgzentry) list;
    pfn_t addr;
    size_t size;
};

#define GFP_LOKERN_ONLY 1
#define GFP_KERN_ONLY   2
#define GFP_HIGH_ONLY   4

#define GFP_LOKERN (GFP_LOKERN_ONLY)
#define GFP_KERN   (GFP_KERN_ONLY | GFP_LOKERN)
#define GFP_HIGH   (GFP_HIGH_ONLY | GFP_KERN)
#define GFP_DEFAULT GFP_KERN

void pginit(void);
pfn_t pgalloc(size_t size, uint8_t type, u_long flags);
void pgfree(pfn_t, size_t);

#define __allocpage(_t) pgalloc(1, (_t), GFP_KERN)
#define __freepage(_p) pgfree((_p), 1)

#define PFN_INVALID ((pfn_t)-1)

#endif
