/*
 * Copyright (c) 2015, Gianluca Guida
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef __fixmems_h
#define __fixmems_h

#include <uk/types.h>
#include <uk/slab.h>

int fixmem_grow(struct slab *sc);
int fixmem_shrink(struct slab *sc);
void *fixmem_alloc_opq(struct slab *sc, void *opq);
void fixmem_free(void *ptr);
int fixmem_register(struct slab *sc, char *name, size_t objsize,
		    void (*ctr) (void *, void *, int), int cachealign);
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
