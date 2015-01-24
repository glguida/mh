#include <uk/types.h>
#include <uk/param.h>
#include <ukern/pfndb.h>
#include <ukern/fixmems.h>
#include <ukern/heap.h>
#include <ukern/vmap.h>
#include <machine/uk/pmap.h>
#include <lib/lib.h>
#include "kern.h"

void arch_init(void);

void
kern_boot(void)
{

    pginit();
    fixmems_init();
    pmap_boot();

    heap_init();
    vmap_init();
    vmap_free(KVA_SVMAP, VMAPSIZE);

    pfndb_printranges();
    pfndb_printstats();
    printf("kernel loaded at va %08lx:%08lx\n", UKERNTEXTOFF, UKERNEND);

    arch_init();
}
