#include <uk/types.h>
#include <ukern/vmap.h>
#include <lib/lib.h>
#include "lapic.h"

static void *lapic_base;

void
lapic_init(paddr_t base)
{

    lapic_base = kvmap(base, LAPIC_SIZE);
    dprintf("lapic (PA: %016llx) mapped at addr %p\n", base, lapic_base);
}


