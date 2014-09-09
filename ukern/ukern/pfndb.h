#ifndef __uk_pfndb_h
#define __uk_pfndb_h

#include <ukern/gfp.h>
#include <ukern/slab.h>

typedef struct {
    uint8_t type;
    union {
	char           ptr[0];
	struct pgzone  pz;
	struct slab    sh;
    };
} __packed ipfn_t;

enum { 
  /* Invalid must be zero, i.e. status of blank page. */
  PFNT_INVALID = 0,

  PFNT_HEAP,             /* Kernel heap */
  PFNT_FREE_PZ_LONE,     /* GFP unique pagezone */
  PFNT_FREE_PZ_STRT,     /* GFP pagezone start */
  PFNT_FREE_PZ_TERM,     /* GFP pagezone end */
  PFNT_FIXMEM,           /* Fixed memory slab allocation */
  PFNT_USER,             /* User allocated pages */

  /* Keep these two at the highest value, for overlap checking */
  PFNT_FREE,		 /* Free Page */
  PFNT_SYSTEM,		 /* Generic System Page */
  PFNT_MMIO,	         /* I/O Mapped Page */
  PFNT_NUM
};

extern unsigned pfndb_max;
#define pfndb_max() pfndb_max

void pfndb_clear(unsigned);
void pfndb_add(unsigned, uint8_t);
void pfndb_subst(uint8_t, uint8_t);

void  pfndb_settype(unsigned, uint8_t);
uint8_t pfndb_type(unsigned);
void pfndb_printstats(void);
void pfndb_printranges(void);

void *pfndb_getptr(unsigned);
void *pfndb_setup(void *, unsigned);

#endif
