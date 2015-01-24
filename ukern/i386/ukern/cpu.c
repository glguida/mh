#include <uk/types.h>
#include <machine/uk/cpu.h>
#include <lib/lib.h>
#include "lapic.h"

int number_cpus = 0;
struct cpuinfo cpus[UKERN_MAX_CPUS];
int cpu_phys_to_id[UKERN_MAX_PHYSCPUS] = { -1,};

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
