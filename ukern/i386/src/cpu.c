#include <uk/types.h>
#include <machine/uk/lapic.h>
#include <machine/uk/cpu.h>
#include <machine/uk/param.h>
#include <machine/uk/i386.h>
#include <uk/heap.h>
#include <lib/lib.h>



int number_cpus = 0;
int cpu_phys_to_id[UKERN_MAX_PHYSCPUS] = { -1, };
struct cpu_info *cpus[UKERN_MAX_CPUS] = { 0, };


extern char _ap_start, _ap_end;

struct cpu_info *cpuinfo_get(unsigned id)
{

	if (id >= UKERN_MAX_CPUS) {
		dprintf("CPU ID %02d too big\n", id);
		return NULL;
	} else if (id >= number_cpus) {
		dprintf("Requested non-active cpu %d\n", id);
		return NULL;
	}
	return cpus[id];
}

int cpu_setup(int);
int cpu_add(uint16_t physid, uint16_t acpiid)
{
	int id;
	struct cpu_info *cpuinfo;

	if (physid >= UKERN_MAX_PHYSCPUS) {
		printf("CPU Phys ID %02x too big. Skipping.\n", physid);
		return -1;
	}

	if (number_cpus >= UKERN_MAX_CPUS) {
		printf("Too many CPUs. Skipping.\n");
		return -1;
	}

	id = number_cpus++;
	dprintf("Adding CPU %d (P:%d, A:%d)\n", id, physid, acpiid);

	cpuinfo = heap_alloc(sizeof(struct cpu_info));
	cpuinfo->cpu_id = id;
	cpuinfo->thread = NULL;
	cpuinfo->cpu = cpu_setup(id);
	cpuinfo->phys_id = physid;
	cpuinfo->acpi_id = acpiid;
	cpuinfo->tss.iomap = 108;
	cpuinfo->self = cpuinfo;
	cpus[id] = cpuinfo;
	cpu_phys_to_id[physid] = id;


	return id;
}

int cpu_number_from_lapic(void)
{
	unsigned physid, id;
	extern int cpu_phys_to_id[UKERN_MAX_PHYSCPUS];

	physid = lapic_getcurrent();
	dbgassert(physid < UKERN_MAX_PHYSCPUS);
	id = cpu_phys_to_id[physid];
	dbgassert(id < UKERN_MAX_CPUS);
	return id;
}

void cpu_wakeup_aps(void)
{
	int i;
	struct cpu_info *cpu;
	unsigned cpuid = cpu_number_from_lapic();

	cmos_write(0xf, 0xa);	// Shutdown causes warm reset.

	for (i = 0; i < number_cpus; i++) {
		if (i == cpuid)
			continue;
		cpu = cpuinfo_get(i);

		printf("Waking up CPU %02d...", i);

		/* Setup AP bootstrap page */
		memcpy((void *) (UKERNBASE + UKERN_APBOOT(i)), &_ap_start,
		       (size_t) & _ap_end - (size_t) & _ap_start);

		/* Setup Warm Reset Vector */
		*(uint16_t *) (UKERNBASE + 0x467) = UKERN_APBOOT(i) & 0xf;
		*(uint16_t *) (UKERNBASE + 0x469) = UKERN_APBOOT(i) >> 4;

		/* INIT-SIPI-SIPI sequence. */
		lapic_ipi(cpu->phys_id, APIC_DLVR_INIT, 0);
		_delay();
		lapic_ipi(cpu->phys_id, APIC_DLVR_START,
			  UKERN_APBOOT(i) >> 12);
		_delay();
		lapic_ipi(cpu->phys_id, APIC_DLVR_START,
			  UKERN_APBOOT(i) >> 12);
		_delay();
		printf(" done\n");
	}
}
