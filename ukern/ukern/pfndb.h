#ifndef __uk_pfndb_h
#define __uk_pfndb_h

typedef struct {
    uint8_t type;
    void *slab;
} __packed ipfn_t;

enum {
  PFN_FREE,		/* Free Page */
  PFN_SYSTEM,		/* Generical System Page */
  PFN_IOMAP,		/* I/O Mapped Page */
  PFN_PTABLE,            /* Page tables */
  PFN_FREE_PZ_LONE,	/* GFP unique pagezone */
  PFN_FREE_PZ_STRT,	/* GFP pagezone start */
  PFN_FREE_PZ_TERM,	/* GFP pagezone end */
  PFN_SYS_MSLAB,		/* Meta Slab Page */
  PFN_SYS_SLAB,		/* Slab Page */
  PFN_USER,		/* User allocated pages */

  PFN_NUMTYPES
};

void pfndb_inittype(int, uint8_t);
uint8_t pfndb_type(int);
void pfndb_setslab(int, void *);
void *pfndb_getslab(int);
void pfndb_printstats(void);

void _setpfndb(void *);

#endif
