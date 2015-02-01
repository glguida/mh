#ifndef __fixmems_h
#define __fixmems_h

#include <uk/types.h>
#include <ukern/slab.h>

int fixmem_grow(struct slab *sc);
int fixmem_shrink(struct slab *sc);
void *fixmem_alloc_opq(struct slab *sc, void *opq);
void fixmem_free(void *ptr);
int fixmem_register(struct slab *sc, char *name, size_t objsize,
		     void (*ctr)(void *, void *, int), int cachealign);
void fixmem_deregister(struct slab *sc);
void fixmem_dumpstats(void);

#define setup_fixmem(_sc, _sz)					\
  fixmem_register((_sc), "fixmem" #_sz, (_sz), NULL, 0)
#define fini_fixmem(_sc)			\
  fixmem_deregister((_sc))


extern struct slab m4k;
extern struct slab m12k;

void fixmems_init();

#define alloc4k() fixmem_alloc_opq(&m4k, NULL)
#define free4k(_p) fixmem_free(_p)

#define alloc12k() fixmem_alloc_opq(&m12k, NULL)
#define free12k(_p) fixmem_free(_p)

#endif
