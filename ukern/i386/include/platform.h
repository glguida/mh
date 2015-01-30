#ifndef _i386_platform_h
#define _i386_platform_h

void platform_init(void);

static inline void
platform_wait(void)
{

    asm volatile("sti\n\thlt");
}

#endif
