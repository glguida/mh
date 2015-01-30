#include <uk/types.h>
#include <ukern/structs.h>
#include "addrspc.h"

static struct slab addrspcs;

void
addrspc_init(void)
{

    setup_structcache(&addrspcs, addrspc);
}

struct addrspc *
addrspc_boot(struct pmap *pmap)
{
    struct addrspc *as;

    as = structs_alloc(&addrspcs);
    as->pmap = pmap;
    return as;
}
