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


#ifndef _i386_cpu_h
#define _i386_cpu_h

#ifndef _ASSEMBLER
#include <uk/types.h>
#include <uk/assert.h>
#include <uk/queue.h>
#include <machine/uk/lapic.h>
#include <machine/uk/i386.h>

#define UKERN_MAX_CPUS 64
#define UKERN_MAX_PHYSCPUS 64

struct cpu_info {
	/* Must be first */
	uint32_t cpu_id;	/* fs:0 */
	struct thread *thread;	/* fs:4 */
	struct cpu *cpu;

	int tlbop; /* TLB shootdown */

	uint16_t phys_id;
	uint16_t acpi_id;
	struct tss tss;
	struct cpu_info *self;
};

extern cpumask_t cpus_active;

int cpu_add(uint16_t physid, uint16_t acpiid);
struct cpu_info *cpuinfo_get(unsigned id);
void cpu_wakeup_aps(void);
int cpu_number_from_lapic(void);

void cpu_nmi(int cpu);
void cpu_nmi_mask(cpumask_t map);
void cpu_nmi_broadcast(void);
void cpu_ipi(int cpu, uint8_t vct);
void cpu_ipi_mask(cpumask_t map, uint8_t vct);
void cpu_ipi_broadcast(uint8_t vct);

#define foreach_cpumask(__mask, __op)					\
	do {								\
		int i = 0;						\
		cpumask_t m = (__mask) & cpus_active;			\
									\
		/* https://xkcd.com/292/ */				\
		/* allow safe use of continue */			\
		goto _start;						\
		while (m != 0) {					\
			i++;						\
			m >>= 1;					\
		_start:							\
			if (m & 1) __op;				\
		};							\
	} while(0)

static inline struct cpu_info *current_cpuinfo(void)
{
	struct cpu_info *ptr;

	asm volatile ("movl %%fs:0, %0\n":"=r" (ptr));
	return ptr;
}

static inline int cpu_number(void)
{
	struct cpu_info *ptr;

	asm volatile ("movl %%fs:0, %0\n":"=r" (ptr));
        return ptr->cpu_id;
}

static inline struct cpu *current_cpu(void)
{
	struct cpu_info *ptr;

	asm volatile ("movl %%fs:0, %0\n":"=r" (ptr));
	return ptr->cpu;
}

static inline struct thread *current_thread(void)
{
	struct cpu_info *ptr;

	asm volatile ("movl %%fs:0, %0\n":"=r" (ptr));
	return ptr->thread;
}

static inline void set_current_thread(struct thread *t)
{
	current_cpuinfo()->thread = t;
}


#else				/* _ASSEMBLER */

#define CPU_NUMBER(_reg)			\
    movl %fs:0, _reg;				\
    movl (_reg), _reg

#define CURRENT_THREAD(_reg)			\
    movl %fs:0, _reg;				\
    movl 4(_reg), _reg

#endif

#endif
