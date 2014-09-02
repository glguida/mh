#ifndef __ukern_gfp_h
#define __ukern_gfp_h

#include <uk/queue.h>

typedef struct pgzone {
    LIST_ENTRY(pgzone) list;
    u_long type;
    u_long ppn;
    u_long size;
} pgzone_t;

#define GFP_LOKERN_ONLY 1
#define GFP_KERN_ONLY   2
#define GFP_HIGH_ONLY   4

#define GFP_LOKERN (GFP_LOKERN_ONLY)
#define GFP_KERN   (GFP_KERN_ONLY | GFP_LOKERN)
#define GFP_HIGH   (GFP_HIGH_ONLY | GFP_KERN)
#define GFP_DEFAULT GFP_KERN

void getfreepages_init(void);
long getfreepages(u_long n, u_long type, u_long flags);
void freepages(u_long ppn, u_long n);

#define getfreepage(_t) getfreepages(1, (_t), GFP_KERN)
#define freepage(_p) freepages((_p), 1)

#endif
