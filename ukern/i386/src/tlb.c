#include <uk/types.h>
#include <machine/uk/tlb.h>


void __flush_local_tlbs(void)
{

	asm volatile ("mov %%cr3, %%eax; mov %%eax, %%cr3\n":::"eax");
}

void __flush_global_tlbs(void)
{

	asm volatile ("mov %%cr4, %%eax\n"
		      "and $0xffffff7f, %%eax\n"
		      "mov %%eax, %%cr4\n"
		      "or  $0x00000080, %%eax\n"
		      "mov %%eax, %%cr4\n":::"eax");
}

void __flush_tlbs(cpumask_t cpumask, unsigned flags)
{

	if (flags & TLBF_GLOBAL) {
		/* XXX: IPI ALL BUT SELF */
		__flush_global_tlbs();
	} else {
		/* XXX: IPI CPUMASK BUT SELF */
		__flush_local_tlbs();
	}
}
