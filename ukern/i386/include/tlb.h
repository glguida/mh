#ifndef __i386_tlb_h
#define __i386_tlb_h

#include <machine/uk/pmap.h>

#define TLBF_NULL   0
#define TLBF_LOCAL  1
#define TLBF_GLOBAL 2

#define restricts_permissions(_o, _n) 1

static __inline int
__tlbflushp(l1e_t ol1e, l1e_t nl1e)
{

    /* Previous not present. Don't flush. */
    if (!l1eflags(ol1e) & PG_P)
	return 0;

    /* Mapping a different page. Flush. */
    if (l1epfn(ol1e) != l1epfn(nl1e))
	goto flush;

    if (restricts_permissions(ol1e, nl1e))
	goto flush;

    return 0;

  flush:
    if ((l1eflags(ol1e) & PG_G) || (l1eflags(nl1e) & PG_G))
	return TLBF_GLOBAL;

    return TLBF_LOCAL;
}

void __flush_tlbs(cpumask_t cpu, unsigned flags);
void __flush_local_tlbs(void);
void __flush_global_tlbs(void);

#endif
