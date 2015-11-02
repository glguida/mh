#include <uk/types.h>
#include <uk/fixmems.h>

struct slab m4k;
struct slab m12k;

void fixmems_init()
{

	setup_fixmem(&m4k, 4096);
	setup_fixmem(&m12k, 12 * 1024);
}
