#include <uk/types.h>
#include <machine/uk/tlb.h>
#include <machine/uk/cpu.h>

static int __tlbcnt = 0;

void __flush_tlbs_on_nmi(void)
{
	int tlbf = __sync_fetch_and_and(&current_cpuinfo()->tlbop, 0);


	if (tlbf & TLBF_GLOBAL)
		__flush_global_tlbs();
	else if (tlbf & TLBF_NORMAL)
		__flush_local_tlbs();

	__sync_sub_and_fetch(&__tlbcnt, 1);
}


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

void __flush_tlbs(cpumask_t cpumask, unsigned tlbf)
{

	foreach_cpumask(cpumask, {
			struct cpu_info *ci;

			if (i == cpu_number())
				continue;

			ci = cpuinfo_get(i);
			if (ci == NULL)
				continue;
			__sync_fetch_and_or(&ci->tlbop, tlbf);
			__sync_add_and_fetch(&__tlbcnt, 1);
			cpu_nmi(i);
		});

	if (tlbf & TLBF_GLOBAL)
		__flush_global_tlbs();
	else if (tlbf & TLBF_NORMAL)
		__flush_local_tlbs();

	/* Synchronous wait */
	while (__sync_add_and_fetch(&__tlbcnt, 0) != 0) {
		asm volatile ("pause; pause; pause;");
	}
}
