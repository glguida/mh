#include <uk/types.h>
#include <uk/param.h>
#include <machine/uk/pmap.h>
#include <machine/uk/cpu.h>
#include <ukern/pfndb.h>
#include <ukern/fixmems.h>
#include <ukern/heap.h>
#include <ukern/vmap.h>
#include <ukern/thread.h>
#include <acpica/acpica.h>
#include <lib/lib.h>
#include <ukern/kern.h>
#include "i386.h"

char *_boot_cmdline = NULL;

void _load_segs(unsigned int, struct tss *, struct cpu_info **cpu);

struct e820e {
	uint64_t addr;
	uint64_t len;
	uint32_t type;
	uint32_t acpi;
} __packed *p;

void platform_init(void)
{
	acpi_findrootptr();
	acpi_init();
	pic_off();
}

#define E820_START       ((struct e820e *)UKERN_BSMAP16)
#define E820_FOREACH(_p)				\
    for ((_p) = E820_START; p->type || p->len || p->addr; p++)

void sysboot(void)
{
	void *lpfndb;
	unsigned i, cpuid;
	struct pmap *pmap;
	uint32_t pfn_maxmem = 0;
	uint32_t pfn_maxaddr = 0;

	if (_boot_cmdline != NULL)
		printf("\ncmdline: %s\n\n", _boot_cmdline);

	/* Search for maxpfns */
	E820_FOREACH(p) {
		uint64_t lpfn;
		lpfn = (p->addr + p->len) >> PAGE_SHIFT;

		printf("\t%016llx:%016llx -> %10s(%ld)\n",
		       p->addr, p->len,
		       p->type == 1 ? "Memory" : "Reserved",
		       (long) p->type);

		if (lpfn > pfn_maxaddr)
			pfn_maxaddr = lpfn;

		if (p->type == 1 && lpfn > pfn_maxmem)
			pfn_maxmem = lpfn;
	}
	printf("max PFNs: mem %8x addr %8x\n", pfn_maxmem, pfn_maxaddr);

	/* Initialise PFNDB */
	lpfndb = pfndb_setup((void *) UKERN_PFNDB, pfn_maxaddr);
	E820_FOREACH(p) {
		int t = p->type == 1 ? PFNT_FREE : PFNT_MMIO;
		int pfn = atop(trunc_page(p->addr));
		int lpfn = atop(round_page(p->addr + p->len));

		for (; pfn < lpfn; pfn++)
			pfndb_add(pfn, t);
	}

	/* No need to mark 0 page as SYSTEM, given all there's in it */

	/* Mark holes as MMIO */
	pfndb_subst(PFNT_INVALID, PFNT_MMIO);

	/* Mark kernel as SYSTEM */
	for (i = 0; i <= atop(round_page(UKERNEND - UKERNBASE)); i++)
		pfndb_add(i, PFNT_SYSTEM);

	/* Mark PFNDB as SYSTEM */
	for (i = atop(UKERN_PFNDB - UKERNBASE);
	     i <= atop(trunc_page((unsigned long) lpfndb - UKERNBASE));
	     i++)
		pfndb_add(i, PFNT_SYSTEM);

	pginit();
	fixmems_init();
	pmap_init();

	printf("Booting...\n");
	pmap = pmap_boot();

	heap_init();
	vmap_init();
	vmap_free(KVA_SVMAP, VMAPSIZE);

	platform_init();
	__insn_barrier();	/* LAPIC now mapped */

	/* Finish up initialization quickly.
	   We can now setup per-cpu data. */
	cpuid = cpu_number_from_lapic();
	_load_segs(cpuid, &cpu_get(cpuid)->tss, &cpu_get(cpuid)->self);
	__insn_barrier();	/* FS: now valid */

	kern_boot();
}

void sysboot_ap(void)
{
	unsigned cpuid;
	struct pmap *pmap;
	struct addrspc *as;

	pmap = pmap_boot();
	__insn_barrier();	/* LAPIC now mapped */

	cpuid = cpu_number_from_lapic();
	_load_segs(cpuid, &cpu_get(cpuid)->tss, &cpu_get(cpuid)->self);
	__insn_barrier();	/* FS: now valid */
	printf("CPU %d on.\n", cpu_number());

	kern_bootap();
}
