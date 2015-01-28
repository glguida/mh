#ifndef _i386_i386_h
#define _i386_i386_h

static void
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

#endif
