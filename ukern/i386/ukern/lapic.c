#include <uk/types.h>
#include <ukern/vmap.h>
#include <lib/lib.h>
#include <machine/uk/lapic.h>

void *lapic_base = NULL;

void lapic_init(paddr_t base)
{

	lapic_base = kvmap(base, LAPIC_SIZE);
	dprintf("lapic (PA: %016llx) mapped at addr %p\n", base,
		lapic_base);
}
