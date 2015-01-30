#ifndef _i386_i386_h
#define _i386_i386_h

#include <uk/types.h>

struct tss {
	uint16_t ptl, tmp0;
	uint32_t esp0;
	uint16_t ss0, tmp1;
	uint32_t esp1;
	uint16_t ss1, tmp2;
	uint32_t esp2;
	uint16_t ss2, tmp3;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint16_t es, tmp4;
	uint16_t cs, tmp5;
	uint16_t ss, tmp6;
	uint16_t ds, tmp7;
	uint16_t fs, tmp8;
	uint16_t gs, tmp9;
	uint16_t ldt, tmpA;
	uint16_t t_flag, iomap;
} __packed;

static inline void
_delay(void)
{
    int i;

    for (i = 0; i < 10000; i++)
	asm volatile ("pause");
}

static inline void
cmos_write(uint16_t addr, uint16_t val)
{
    asm volatile ("outb %0, $0x70\n\t"
		  "outb %0, $0x71\n\t"
		  :: "r"(addr), "r"(val));
}

static inline void
pic_off(void)
{
    asm volatile ("outb %0, $0x21\n\t"
		  "outb %0, $0xa1\n\t"
		  :: "r"(0xff));
}

#endif
