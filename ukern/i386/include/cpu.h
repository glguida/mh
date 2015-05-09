#ifndef _i386_cpu_h
#define _i386_cpu_h

#ifndef _ASSEMBLER
#include <uk/types.h>
#include <uk/assert.h>
#include <uk/queue.h>
#include <machine/uk/lapic.h>
#include <i386/ukern/i386.h>

#define UKERN_MAX_CPUS 64
#define UKERN_MAX_PHYSCPUS 64

struct cpu_info {
	/* Must be first */
	uint32_t cpu_id;	/* fs:0 */
	struct thread *thread;	/* fs:4 */
	struct cpu *cpu;

	uint16_t phys_id;
	uint16_t acpi_id;
	struct tss tss;
	struct cpu_info *self;
};

int cpu_add(uint16_t physid, uint16_t acpiid);
struct cpu_info *cpuinfo_get(unsigned id);
void cpu_wakeup_aps(void);
int cpu_number_from_lapic(void);

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
