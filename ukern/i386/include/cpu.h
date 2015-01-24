#ifndef _i386_cpu_h
#define _i386_cpu_h

#include <uk/types.h>

#define UKERN_MAX_CPUS 64
#define UKERN_MAX_PHYSCPUS 64

struct cpuinfo {
    uint16_t phys_id;
    uint16_t acpi_id;
};

struct cpuinfo *cpu_get(unsigned id);
int cpu_add(uint16_t physid, uint16_t acpiid);

#endif
