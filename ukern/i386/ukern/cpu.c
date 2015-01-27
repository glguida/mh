#include <uk/types.h>
#include <machine/uk/lapic.h>
#include <machine/uk/cpu.h>
#include <machine/uk/param.h>
#include <lib/lib.h>
#include "i386.h"

int number_cpus = 0;
struct cpuinfo cpus[UKERN_MAX_CPUS];
int cpu_phys_to_id[UKERN_MAX_PHYSCPUS] = { -1,};

extern char _ap_start, _ap_end;

struct cpuinfo *
cpu_get(unsigned id)
{

    if (id >= UKERN_MAX_CPUS) {
	dprintf("CPU ID %02d too big\n", id); 
	return NULL;
    } else if (id >= number_cpus) {
	dprintf("Requested non-active cpu %d\n", id);
	return NULL;
    }
    return cpus + id;
}

int
cpu_add(uint16_t physid, uint16_t acpiid)
{
    int id;
    struct cpuinfo *cpu;

    if (physid >= UKERN_MAX_PHYSCPUS) {
	printf("CPU Phys ID %02x too big. Skipping.\n", physid);
	return -1;
    }

    if (number_cpus >= UKERN_MAX_CPUS) {
	printf("Too many CPUs. Skipping.\n");
	return -1;
    }

    id = number_cpus++;
    cpu = cpus + id;

    dprintf("Adding CPU %d (P:%d, A:%d)\n", id, physid, acpiid);

    cpu->phys_id = physid;
    cpu->acpi_id = acpiid;

    cpu_phys_to_id[physid] = id;
    return id;
}

void
cpu_wakeup_aps(void)
{
    int i;
    struct cpuinfo *cpu;

    cmos_write(0xf, 0xa); // Shutdown causes warm reset.

    for (i = 0; i < number_cpus; i++) {
	if (i == thiscpu())
	    continue;
	cpu = cpu_get(i);

	printf("Waking up CPU %02d...", i);

	/* Setup AP bootstrap page */
	memcpy((void *)(UKERNBASE + UKERN_APBOOT(i)), &_ap_start,
	       (size_t)&_ap_end - (size_t)&_ap_start);

	/* Setup Warm Reset Vector */
	*(uint16_t *)(UKERNBASE + 0x467) = UKERN_APBOOT(i) & 0xf;
	*(uint16_t *)(UKERNBASE + 0x469) = UKERN_APBOOT(i) >> 4;

	/* INIT-SIPI-SIPI sequence. */
	lapic_ipi(cpu->phys_id, APIC_DLVR_INIT, 0);
	_delay();
	lapic_ipi(cpu->phys_id, APIC_DLVR_START, UKERN_APBOOT(i) >> 12);
	_delay();
	lapic_ipi(cpu->phys_id, APIC_DLVR_START, UKERN_APBOOT(i) >> 12);
	_delay();
	printf(" done\n");
    }
}
