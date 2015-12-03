/*
 * Copyright (c) 2015, Gianluca Guida
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <uk/types.h>
#include <machine/uk/apic.h>
#include <machine/uk/cpu.h>
#include <machine/uk/param.h>
#include <machine/uk/i386.h>
#include <uk/heap.h>
#include <lib/lib.h>

cpumask_t cpus_active = 0;

static int number_cpus = 0;
static int cpu_phys_to_id[UKERN_MAX_PHYSCPUS] = { -1, };
static struct cpu_info *cpus[UKERN_MAX_CPUS] = { 0, };

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

struct cpu *cpu_setup(int);

int cpu_add(uint16_t physid, uint16_t platformid)
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
	dprintf("Adding CPU %d (PHYS:%d, PLAT:%d)\n", id, physid, platformid);

	cpuinfo = heap_alloc(sizeof(struct cpu_info));
	cpuinfo->cpu_id = id;
	cpuinfo->thread = NULL;
	cpuinfo->cpu = cpu_setup(id);
	cpuinfo->phys_id = physid;
	cpuinfo->plat_id = platformid;
	cpuinfo->tss.iomap = 108;
	cpuinfo->self = cpuinfo;
	cpus[id] = cpuinfo;
	cpu_phys_to_id[physid] = id;

	cpus_active |= ((cpumask_t) 1 << id);

	return id;
}

void cpu_nmi(int cpu)
{
	struct cpu_info *ci = cpuinfo_get(cpu);

	if (ci == NULL)
		return;
	lapic_ipi(ci->phys_id, APIC_DLVR_NMI, 0);
}

void cpu_nmi_broadcast(void)
{
	lapic_ipi_broadcast(APIC_DLVR_NMI, 0);
}

void cpu_nmi_mask(cpumask_t map)
{

	foreach_cpumask(map, cpu_nmi(i));
}

void cpu_ipi(int cpu, uint8_t vct)
{
	struct cpu_info *ci = cpuinfo_get(cpu);

	if (ci == NULL)
		return;
	lapic_ipi(ci->phys_id, APIC_DLVR_FIX, vct);
}

void cpu_ipi_broadcast(uint8_t vct)
{

	lapic_ipi_broadcast(APIC_DLVR_FIX, vct);
}

void cpu_ipi_mask(cpumask_t map, uint8_t vct)
{

	foreach_cpumask(map, cpu_ipi(i, vct));
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
