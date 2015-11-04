/*
 * Copyright (c) 2015, Gianluca Guida
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include <uk/types.h>
#include <machine/uk/cpu.h>
#include <machine/uk/param.h>
#include <uk/kern.h>
#include <uk/sys.h>
#include <lib/lib.h>

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

void framedump(struct xcptframe *f)
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

static unsigned vect_to_xcpt(uint32_t vect)
{
	switch (vect) {
	case 14:
		return XCPT_PGFAULT;
	default:
		return -1;
	}
}

int xcpt_entry(uint32_t vect, struct xcptframe *f)
{
	current_thread()->frame = f;

	if (vect == 0x2) {
		/* NMI Handler */
		__flush_tlbs_on_nmi();
		return 0;
	}

	if (f->cs == UCS) {
		thxcpt(vect_to_xcpt(vect));
		die();
	} else {
		if (usrpgfault && (vect == 14)) {
			printf("Jumping!\n");
			_longjmp(current_cpu()->usrpgfaultctx, 1);

		}

		printf("\nException #%2u at %02x:%08x, addr %08x",
		       vect, f->cs, f->eip, f->cr2);

		if (vect < 20)
			printf(": %s\n", exceptions[vect]);
		else
			printf("\n");
		framedump(f);
		goto _hlt;
	}

	return 0;

      _hlt:
	/* XXX: Kernel bug. panic and block other cpus */
	asm volatile ("cli; hlt");
	/* Not reached */
	return -1;
}

int intr_entry(uint32_t vect, struct xcptframe *f)
{
	int usrint = !!(f->cs == UCS);

	struct thread *th = current_thread();

	if (!usrint || vect != 0x80) {
		printf("\nUnhandled interrupt %2u\n", vect);
		framedump(f);
		if (usrint)
			die();
		return 0;
	}

	th->frame = f;
	f->eax = sys_call(f->eax, f->edi, f->esi, f->ecx);
	th->frame = NULL;
	return 0;
}
