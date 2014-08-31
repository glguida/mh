#include <uk/types.h>
#include <lib/lib.h>

struct intframe {
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
  uint32_t esp;
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;
  /* exception stack */
  uint32_t err;
  uint32_t eip;
  uint32_t cs;
  uint32_t eflags;
  uint32_t espx;
  uint32_t ss;
} __packed;

char *exceptions[] = {
  "Divide by zero exception",
  "Debug exception",
  "NMI",
  "Overflow exception",
  "Breakpoint exception",
  "Bound range exceeded",
  "Invalid opcode",
  "No math coprocessor",
  "Double fault",
  "Coprocessor segment overrun",
  "Invalid TSS",
  "Segment not present",
  "Stack segment fault",
  "General protection fault",
  "Page fault",
  "Reserved exception",
  "Floating point error",
  "Alignment check fault",
  "Machine check fault",
  "SIMD Floating-Point Exception",
};

void framedump(struct intframe *f)
{

    printf("\tCR3: %08x\tCR2: %08x\terr: %08x\n",
	   f->cr3, f->cr2, f->err);
    printf("\tCS: %04x\tEIP: %08x\tEFLAGS: %08x\n",
	   (int)f->cs, f->eip, f->eflags);
    printf("\tEAX: %08x\tEBX: %08x\tECX: %08x\tEDX:%08x\n",
	   f->eax, f->ebx, f->ecx, f->edx);
    printf("\tEDI: %08x\tESI: %08x\tEBP: %08x\tESP:%08x\n",
	   f->edi, f->esi, f->ebp, f->espx);
    printf("\tDS: %04x\tES: %04x\tFS: %04x\tGS: %04x\n",
	   f->ds, f->es, f->fs, f->gs);
}

int
xcpt_entry(uint32_t vect, struct intframe *f)
{

    printf("\nException #%2u at %02x:%08x", vect, f->cs, f->eip);
    if (vect < 20) 
	printf(": %s\n", exceptions[vect]);
    else
	printf("\n");

    framedump(f);
    return 0;
}

int
intr_entry(uint32_t vect, struct intframe *f)
{

    printf("\nUnknown exception %2u\n", vect);
    framedump(f);
    return 0;
}
