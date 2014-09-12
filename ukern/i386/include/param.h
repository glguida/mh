#ifndef __i386_param_h
#define __i386_param_h

#ifdef _UKERNEL

#ifdef _ASSEMBLER
#define __ULONG(_c) _c
#else
#define __ULONG(_c) ((unsigned long)(_c))
#endif

#include <machine/uk/vmparam.h>

#define MAXCPUS         32

/* i386 Virtual Address Space Map:
 *
 * 00000000:bfffffff     User mappings
 * c0000000:UKERNEND     Kernel code and data
 * c0000000:DMAP_END     Kernel 1:1 direct mapping
 * DMAP_END:ffe00000     Kernel VA allocation range
 */

#define UKERNBASE __ULONG(0xc0000000)
#define UKERNEND  _ukern_end
#define DMAPSIZE  __ULONG(768 << 20)
#define KVA_SDMAP __ULONG(UKERNBASE)
#define KVA_EDMAP __ULONG(UKERNBASE + DMAPSIZE)
#define KVA_SVMAP __ULONG(KVA_EDMAP)
#define KVA_EVMAP __ULONG(0xffe00000)
#define VMAPSIZE  __ULONG(KVA_EVMAP - KVA_SVMAP)

#define UKERNCMPOFF     0x100000 /* 1Mb */
#define UKERNTEXTOFF    (UKERNBASE + UKERNCMPOFF)

/*
 * Physical address ranges:
 *
 * 00000000:00100000     Boot-time allocation memory (640k)
 *                       + BIOS area
 * 00100000:_end         Kernel code + data
 * _end    :LKMEMEND     Low 1:1 Kernel memory
 * KMEMSTRT:KMEMEND      1:1 Kernel memory
 * HIGHSTRT:HIGHEND      High memory, accessed through mapping
 */

#define LKMEMSTRT        0
#define LKMEMEND         __ULONG(16 << 20)
#define KMEMSTRT         LKMEMEND
#define KMEMEND          DMAPSIZE
#define HIGHSTRT         DMAPSIZE

/* Temporary physical boot-time allocation physical address */
#define UKERN_BCODE16   0x10000 /* 16bit code */
#define UKERN_BSTCK16   0x2fffe /* 16bit stack */
#define UKERN_BSMAP16   0x30000 /* E820 map */

#define UKERN_BL3TABLE  0x50000 /* Temporary L3 table */
#define UKERN_BL2TABLE  0x51000 /* Temporary L2 table */
#define UKERN_BGDTREG   0x52000 /* Temporary GDT reg */
#define UKERN_BGDTABLE  0x52030 /* Temporary GDT */

/* Non-temporary boot-time static memory allocation */
#define UKERN_PFNDB     (((UKERNEND + 127) >> 7) << 7)


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
