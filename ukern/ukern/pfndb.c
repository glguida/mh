#include <uk/types.h>
#include <uk/assert.h>
#include <lib/lib.h>

#include <ukern/pfndb.h>

static ipfn_t *pfndb = NULL;
static u_long pfndb_stats[PFN_NUMTYPES];
static const char *pfndb_names[PFN_NUMTYPES] = {
    "Free Pages",
    "System Pages",
    "I/O Mapped Pages",
    "Page Tables",
    "One-Page Pagezones",
    "Pagezone Starting Pages",
    "Pagezone Terminal Pages",
    "Metaslab Pages",
    "Slab Pages",
    "User Pages",
};

static void
pfndb_stats_inctype(unsigned int t)
{
    assert(t < PFN_NUMTYPES);
    pfndb_stats[t]++;
}

static void
pfndb_stats_dectype(unsigned int t)
{

    if (t < PFN_NUMTYPES)
	pfndb_stats[t]--;
}

void
pfndb_inittype(int pfn, uint8_t t)
{
    ipfn_t *p = &pfndb[pfn];

    pfndb_stats_inctype(t);
    p->type = t;
}


void
pfndb_settype(int pfn, uint8_t t)
{
    ipfn_t *p = &pfndb[pfn];

    pfndb_stats_dectype(p->type);
    pfndb_stats_inctype(t);
    p->type = t;
}

uint8_t
pfndb_type(int pfn)
{
    ipfn_t *p = &pfndb[pfn];

    return p->type;
}

void
pfndb_setslab(int pfn, void *s)
{
    ipfn_t *p = &pfndb[pfn];

    p->slab = s;
}

void *
pfndb_slab(int pfn)
{
    ipfn_t *p = &pfndb[pfn];

    return p->slab;
}

#if 0
pagezone_t *
pfndb_pzptr(int pfn)
{
    ipfn_t *p = &pfndb[pfn];

    return &(p->pz);
}
#endif

void
pfndb_printstats(void)
{
    int i;

    printf("PFN Database stats:\n");
    for (i = 0; i < PFN_NUMTYPES; i++)
	printf("\t%20s:%20ld\n",
	       pfndb_names[i],
	       pfndb_stats[i]);
}

void
_setpfndb(void *_pfndb)
{

    pfndb = _pfndb;
}
