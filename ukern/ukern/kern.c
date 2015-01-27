#include <uk/types.h>
#include <uk/param.h>
#include <ukern/pfndb.h>
#include <ukern/fixmems.h>
#include <ukern/heap.h>
#include <ukern/vmap.h>
#include <machine/uk/pmap.h>
#include <lib/lib.h>
#include "kern.h"

#include <machine/uk/cpu.h>

void
kern_bootap(void)
{
}

void
kern_boot(void)
{
    
    pfndb_printranges();
    pfndb_printstats();
    printf("kernel loaded at va %08lx:%08lx\n", UKERNTEXTOFF, UKERNEND);
}
