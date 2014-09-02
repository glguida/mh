#ifndef __structs_h
#define __structs_h

#include <uk/queue.h>
#include <ukern/slab.h>

int structs_grow(struct slab *);
int structs_shring(struct slab *);
void * structs_alloc_opq(struct slab *, void *);
void structs_free(void *);
int structs_register(struct slab *sc, char *name, size_t objsize,
		     void (*ctr)(void *, void *, int), int cachealign);
void structs_deregister(struct slab *sc);
void structs_dumpstats(void);

#define structs_alloc(_sc) structs_alloc_opq(_sc, NULL)
#define setup_structcache(_sc, _type)					\
    structs_register((_sc), #_type, sizeof(struct _type), NULL, 1)
#define fini_structcache(_sc)			\
    structs_deregister((_sc))

#endif
