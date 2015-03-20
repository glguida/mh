#include <uk/types.h>
#include <machine/uk/cpu.h>
#include <machine/uk/param.h>
#include <ukern/kern.h>
#include <uk/sys.h>
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
	       (int) f->cs, f->eip, f->eflags);
	printf("\tEAX: %08x\tEBX: %08x\tECX: %08x\tEDX:%08x\n",
	       f->eax, f->ebx, f->ecx, f->edx);
	printf("\tEDI: %08x\tESI: %08x\tEBP: %08x\tESP:%08x\n",
	       f->edi, f->esi, f->ebp, f->espx);
	printf("\tDS: %04x\tES: %04x\tFS: %04x\tGS: %04x\n",
	       f->ds, f->es, f->fs, f->gs);
}

int xcpt_entry(uint32_t vect, struct intframe *f)
{

	printf("\nException #%2u at %02x:%08x, addr %08x",
	       vect, f->cs, f->eip, f->cr2);
	if (vect < 20)
		printf(": %s\n", exceptions[vect]);
	else
		printf("\n");

	if (f->cs == UCS) {
		int rc;

		switch (vect) {
		case 14:
			rc = thpgfault(f->cr2, 0 /*XXX: */ );
			if (!rc)
				return 0;
			goto _usrerr;
		default:
		      _usrerr:
			/* XXX: kill process instead */
			printf("unhandled exception!\n");
			framedump(f);
			die();
			/* Not reached */
		}
	} else {
		/* XXX: Kernel bug. panic and block other cpus */
		framedump(f);
		goto _hlt;
	}

	return 0;

      _hlt:
	asm volatile ("cli; hlt");
	/* Not reached */
	return -1;
}

int intr_entry(uint32_t vect, struct intframe *f)
{
	int ret;
	int usrint = !!(f->cs == UCS);


	if (!usrint || vect != 0x80) {
		printf("\nUnhandled interrupt %2u\n", vect);
		framedump(f);
		if (usrint)
			die();
		return 0;
	}

	current_thread()->frame = f;
	switch (f->eax) {
	case SYS_PUTC:
		ret = sys_putc(f->edi);
		break;
	case SYS_DIE:
		ret = sys_die();
		break;
	case SYS_XCPTENTRY:
		ret = sys_xcptentry(f->edi, f->esi);
		break;
	default:
		ret = -1;
		break;
	}

	f->eax = ret;
	current_thread()->frame = NULL;
	return 0;
}
