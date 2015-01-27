#ifndef _i386_cpu_h
#define _i386_cpu_h

#include <uk/types.h>
#include <uk/assert.h>
#include <machine/uk/lapic.h>

#define UKERN_MAX_CPUS 64
#define UKERN_MAX_PHYSCPUS 64

struct cpuinfo {
    uint16_t phys_id;
    uint16_t acpi_id;
};

struct cpuinfo *cpu_get(unsigned id);
int cpu_add(uint16_t physid, uint16_t acpiid);
void cpu_wakeup_aps(void);

static inline
int thiscpu(void)
{
    unsigned physid, id;
    extern int cpu_phys_to_id[UKERN_MAX_PHYSCPUS];

    physid = lapic_getcurrent();
    dbgassert(physid < UKERN_MAX_PHYSCPUS);
    id = cpu_phys_to_id[physid];
    dbgassert(id < UKERN_MAX_CPUS);
    return id;
}

#endif
