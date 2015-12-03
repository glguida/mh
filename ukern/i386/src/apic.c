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
#include <uk/heap.h>
#include <uk/vmap.h>
#include <lib/lib.h>
#include <machine/uk/apic.h>

void *lapic_base = NULL;
unsigned lapics_no;
struct lapic_desc {
	unsigned physid;
	unsigned platformid;
	uint32_t lint[2];
} *lapics;
unsigned ioapics_no;
struct ioapic_desc {
	void *base;
	unsigned irq;
	unsigned pins;
} *ioapics;


/* Memory Registers */
#define IO_REGSEL 0x00
#define IO_WIN    0x10

/* I/O Registers */
#define IO_ID		0x00
#define IO_VER		0x01
#define IO_ARB		0x02
#define IO_PIN_LO(x) 	(0x10 + 2*(x))
#define IO_PIN_HI(x)	(0x11 + 2*(x))

void lapic_init(paddr_t base, unsigned no)
{
	lapic_base = kvmap(base, LAPIC_SIZE);
	lapics = heap_alloc(sizeof(struct lapic_desc) * no);
	lapics_no = no;
	dprintf("LAPIC PA: %08llx VA: %p\n", base, lapic_base);
}

int lapic_add(uint16_t physid, uint16_t plid)
{
	static unsigned i = 0;

	lapics[i].physid = physid;
	lapics[i].platformid = plid;
	lapics[i].lint[0] = 0;
	lapics[i].lint[1] = 0;
	i++;
}

void lapic_add_nmi(uint8_t pid, int l)
{
	int i;

	if (pid = 0xff) {
		for (i = 0; i < lapics_no; i++)
			lapics[i].lint[l] = (1L << 16)|(APIC_DLVR_NMI << 8);
		return;
	}
	for (i = 0; i < lapics_no; i++) {
		if (lapics[i].platformid == pid) {
			if (l) l = 1;
			lapics[i].lint[l] = (1L << 16)|(APIC_DLVR_NMI << 8);
			return;
		}
	}
	printf("Warning: LAPIC NMI for non-existing platform ID %d\n", pid);
}

void lapic_configure(void)
{
	unsigned i, physid = lapic_getcurrent();
	struct lapic_desc *d = NULL;

	for (i = 0; i < lapics_no; i++) {
		if (lapics[i].physid == physid)
			d = lapics + i;
	}
	if (d == NULL) {
		printf("Warning: Current CPU not in Platform Tables!\n");
		/* Try to continue, ignore the NMI configuration */
	} else  {
		lapic_write(L_LVT_LINT(0), d->lint[0]);
		lapic_write(L_LVT_LINT(1), d->lint[1]);
	}
	/* Enable LAPIC */
	lapic_write(L_MISC, lapic_read(L_MISC) | 0x100);
}

void lapic_platform_done(void)
{
	int i;

	for (i = 0; i < lapics_no; i++)
		cpu_add(lapics[i].physid, lapics[i].platformid);
	/* Since we're here, configure LAPIC of BSP */
	lapic_configure();
}

void ioapic_init(unsigned no)
{

	ioapics = heap_alloc(sizeof(struct ioapic_desc) * no);
	ioapics_no = no;
}

static void ioapic_write(unsigned i, uint8_t reg, uint32_t val)
{
	volatile uint32_t *regsel = (uint32_t *)(ioapics[i].base + IO_REGSEL);
	volatile uint32_t *win = (uint32_t *)(ioapics[i].base + IO_WIN);

	*regsel = reg;
	*win = val;
}

static uint32_t ioapic_read(unsigned i, uint8_t reg)
{
	volatile uint32_t *regsel = (uint32_t *)(ioapics[i].base + IO_REGSEL);
	volatile uint32_t *win = (uint32_t *)(ioapics[i].base + IO_WIN);

	*regsel = reg;
	return *win;
}

void ioapic_add(unsigned num, paddr_t base, unsigned irqbase)
{

	ioapics[num].base = kvmap(base, IOAPIC_SIZE);
	ioapics[num].irq = irqbase;
	ioapics[num].pins = 1 + ((ioapic_read(num, IO_VER) >> 16) & 0xff);
	printf("IOAPIC ID: %02d PA: %08llx VA: %p IRQ:%02d PINS: %02d\n",
	       num, base, ioapics[num].base, irqbase, ioapics[num].pins);
}

unsigned ioapic_irqs(void)
{
	unsigned i;
	unsigned lirq, maxirq = 0;

	for (i = 0; i < ioapics_no; i++) {
		lirq = ioapics[i].irq + ioapics[i].pins;
		if (lirq > maxirq)
			maxirq = lirq;
	}

	return maxirq;
}
