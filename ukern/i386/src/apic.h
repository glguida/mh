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


#ifndef _i386_apic_h
#define _i386_apic_h

#define APIC_DLVR_FIX   0
#define APIC_DLVR_PRIO  1
#define APIC_DLVR_SMI   2
#define APIC_DLVR_NMI 	4
#define APIC_DLVR_INIT  5
#define APIC_DLVR_START 6
#define APIC_DLVR_EINT 	7


/*
 * Local APIC.
 */

/* LAPIC registers.  */
#define L_IDREG		0x20
#define L_VER		0x30
#define L_TSKPRIO	0x80
#define L_ARBPRIO	0x90
#define L_PROCPRIO	0xa0
#define L_EOI		0xb0
#define L_LOGDEST	0xd0
#define L_DESTFMT	0xe0
#define L_MISC		0xf0	/* Spurious vector */
#define L_ISR		0x100	/* 256 bit */
#define L_TMR		0x180	/* 256 bit */
#define L_IRR		0x200	/* 256 bit */
#define L_ERR		0x280
#define L_ICR_LO	0x300
#define L_ICR_HI	0x310
#define L_LVT_TIMER	0x320
#define L_LVT_THERM	0x330
#define L_LVT_PFMCNT	0x340
#define L_LVT_LINT(x)	(0x350 + (x * 0x10))
#define L_LVT_ERR	0x370
#define L_TMR_IC	0x380
#define L_TMR_CC	0x390
#define L_TMR_DIV	0x3e0

#define LAPIC_SIZE      (1UL << 12)

extern void *lapic_base;
void lapic_init(paddr_t base, unsigned no);
void lapic_enable(void);

static inline void lapic_write(unsigned reg, uint32_t data)
{

	*((uint32_t *) (lapic_base + reg)) = data;
}

static inline uint32_t lapic_read(unsigned reg)
{

	return *((uint32_t *) (lapic_base + reg));
}

static inline void lapic_ipi(unsigned physid, uint8_t dlvr, uint8_t vct)
{
	uint32_t hi, lo;

	lo = 0x4000 | (dlvr & 0x7) << 8 | vct;
	hi = (physid & 0xff) << 24;

	lapic_write(L_ICR_HI, hi);
	lapic_write(L_ICR_LO, lo);
}

static inline void lapic_ipi_broadcast(uint8_t dlvr, uint8_t vct)
{
	uint32_t lo;

	lo = (dlvr & 0x7) << 8 | vct
		| /*ALLBUTSELF*/ 0xc0000 | /*ASSERT*/ 0x4000;
	lapic_write(L_ICR_HI, 0);
	lapic_write(L_ICR_LO, lo);
}

#define lapic_getcurrent(void)  (lapic_read(L_IDREG) >> 24)

/*
 * I/O APIC
 */

#define IOAPIC_SIZE     (0x20)

extern unsigned ioapics_no;

void ioapic_init(unsigned no);
void ioapic_add(unsigned num, paddr_t base, unsigned irqbase);
unsigned ioapic_irqs(void);

#endif
