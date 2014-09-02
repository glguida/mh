#ifndef __fixmems_h
#define __fixmems_h

#include <uk/types.h>
#include "fixmem.h"
#include <ukern/fixmems.h>

extern struct slab m4k;

void fixmems_init();

#define alloc4k() fixmem_alloc(&m4k, NULL)
#define free4k(_p) fixmem_free(_p)

#endif
