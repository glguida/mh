#ifndef __i386_param_h
#define __i386_param_h

#ifdef _UKERNEL
#include <machine/uk/vmparam.h>

#define MAXCPUS         32

#define UKERNBASE       0xc0000000
#define UKERNEND        _ukern_end

#define UKERNCMPOFF     0x100000 /* 1Mb */
#define UKERNTEXTOFF    (UKERNBASE + UKERNCMPOFF)

/* Temporary address for boot-time allocation. */
#define UKERN_BCODE16   0x10000 /* 16bit code */
#define UKERN_BSTCK16   0x2fffe /* 16bit stack */
#define UKERN_BSMAP16   0x30000 /* E820 map */

#define UKERN_BL3TABLE  0x50000 /* Temporary L3 table */
#define UKERN_BL2TABLE  0x51000 /* Temporary L2 table */
#define UKERN_BGDTREG   0x52000 /* Temporary GDT reg */
#define UKERN_BGDTABLE  0x52030 /* Temporary GDT */

/* Non-temporary boot-time static memory allocation */
#define UKERN_PFNDB  (((UKERNEND + 127) >> 7) << 7)

#define KCS 0x08
#define KDS 0x10
#define CS16  0x18
#define RDS16 0x20

#define SEG16_ADDR(_a) (((_a) >> 4) & 0xf000)
#define OFF16_ADDR(_a) ((_a) & 0xffff)

#ifndef _ASSEMBLER
extern unsigned long _ukern_end;
#endif

#endif /* _UKERNEL */

#endif
