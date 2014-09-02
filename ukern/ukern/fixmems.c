#include <uk/types.h>
#include <ukern/fixmems.h>

struct slab m4k;

void fixmems_init()
{

    setup_fixmem(&m4k, 12);
}
