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
    uint32_t      cpu_id;   /* fs:0 */
    struct thread *current; /* fs:4 */
    struct thread *idle_thread;
    TAILQ_HEAD(, thread) resched;
    struct tss    tss;
    uint64_t      softirq;

    uint16_t      phys_id;
    uint16_t      acpi_id;
    struct cpu_info *self;
};

int cpu_add(uint16_t physid, uint16_t acpiid);
struct cpu_info *cpu_get(unsigned id);
void cpu_wakeup_aps(void);

static inline struct cpu_info *
current_cpu(void)
{
    struct cpu_info *ptr;

    asm volatile("movl %%fs:0, %0\n": "=r"(ptr));
    return ptr;
}

int cpu_number_from_lapic(void);
#define cpu_number() (current_cpu()->cpu_id)
#define current_thread() (current_cpu()->current)
#define set_current_thread(_th) (current_cpu()->current = (_th))


#else /* _ASSEMBLER */

#define CPU_NUMBER(_reg)			\
    movl %fs:0, _reg;				\
    movl (_reg), _reg

#define CURRENT_THREAD(_reg)			\
    movl %fs:0, _reg;				\
    movl 4(_reg), _reg

#endif

#endif

