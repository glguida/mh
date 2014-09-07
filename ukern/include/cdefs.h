#ifndef __uk_cdefs_h
#define __uk_cdefs_h

#include <machine/uk/cdefs.h>
#include <uk/cdefs_elf.h>

#define __packed __attribute__((__packed__))
#define __aligned(x) __attribute__((__aligned__(x)))
#define __section(x) __attribute__((__section__(x)))

#define __insn_barrier() __asm__ volatile("":::"memory")

#define __printflike(fmtarg, vararg)	\
  __attribute__((__format__(__printf__, fmtarg, vararg)))

#define NULL ((void *)0)

#define __UNCONST(c) ((void *)(unsigned long)(const void *)(c))

#endif
