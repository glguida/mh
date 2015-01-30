#ifndef _ukern_addrspc_h
#define _ukern_addrspc_h

#include <machine/uk/pmap.h>

struct addrspc {
    struct pmap *pmap;

};

void addrspc_init(void);
struct addrspc *addrspc_boot(struct pmap *pmap);

static inline void
addrspc_switch(struct addrspc *as)
{

    pmap_switch(as->pmap);
}

#endif
