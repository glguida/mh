#ifndef _i386_uk_sys_h_
#define _i386_uk_sys_h_

#ifndef _ASSEMBLER

#ifdef _UKERNEL
#include <machine/uk/int_types.h>
#else
#include <machine/int_types.h>
#endif

struct xcptframe {
	/* segments */
	uint16_t ds;
	uint16_t es;
	uint16_t fs;
	uint16_t gs;
	/* CRs    */
	uint32_t cr2;
	uint32_t cr3;
	/* pushal */
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t espx;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	/* exception stack */
	uint32_t err;
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	uint32_t esp;
	uint32_t ss;
} __packed;
#endif

#define XCPTFRAME_SIZE (18 * 4)

#endif
