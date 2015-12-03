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
#include "../lib/lib.h"
#include "acpica.h"

/* ACPICA includes */
#include "acpi.h"

ACPI_PHYSICAL_ADDRESS acpi_rdsp;
ACPI_TABLE_MADT *acpi_madt;

char *madt_subtstr[] = {
	"Local APIC",
	"IO APIC",
	"Interrupt Override",
	"NMI source",
	"Local APIC NMI",
	"Local APIC Override",
	"IO SAPIC",
	"Local Sapic",
	"Interrupt Source",
	"Local X2APIC",
	"Local X2APIC NMI",
	"Generic Interrupt",
	"Generic Distributor",
};

#define madt_subtype_str(_st) (_st < 13 ? madt_subtstr[_st] : "RESERVED")
#define ACPI_MAX_CPUS 64
#define APCI_MAX_INTCTRLRS 16

#if _DREX_MACHINE==i386
static int acpi_madt_scan(void)
{
	int len;
	uint8_t type;
	unsigned nioapic = 0;	
	union {
		uint8_t *ptr;
		struct acpi_madt_local_apic *lapic;
		struct acpi_madt_io_apic *ioapic;
	} _;
	void lapic_init(paddr_t);
	int cpu_add(uint16_t physid, uint16_t acpiid);



	lapic_init(acpi_madt->Address);


	/* Search for APICs. Output of this stage is number of CPUs
	   and I/O APICs */
	len = acpi_madt->Header.Length - sizeof(*acpi_madt);
	_.ptr = (uint8_t *) acpi_madt + sizeof(*acpi_madt);
	while (len > 0) {
		type = *_.ptr;
		switch (type) {
		case ACPI_MADT_TYPE_LOCAL_APIC:
			printf("ACPI MADT LOCAL APIC %02d %02d %08x\n",
			       _.lapic->Id, _.lapic->ProcessorId,
			       _.lapic->LapicFlags);
			if (_.lapic->LapicFlags & ACPI_MADT_ENABLED)
				cpu_add(_.lapic->Id, _.lapic->ProcessorId);
			break;
		/* XXX: GIANLUCA: X2APIC? SAPIC?IOAPIC? */
		case ACPI_MADT_TYPE_IO_APIC:
			printf("ACPI MADT I/O APIC %02d %08x, %02d\n",
			       _.ioapic->Id, _.ioapic->Address,
			       _.ioapic->GlobalIrqBase);
			nioapic++;
			break;
		default:
			break;
		}
		len -= *(_.ptr + 1);
		_.ptr += *(_.ptr + 1);
	}
	ioapic_init(nioapic);

	/* Setup I/O APICS. Output of this stage is number of GSIs */
	nioapic = 0;
	len = acpi_madt->Header.Length - sizeof(*acpi_madt);
	_.ptr = (uint8_t *) acpi_madt + sizeof(*acpi_madt);
	while (len > 0) {
		type = *_.ptr;
		switch (type) {
		/* XXX: GIANLUCA: X2APIC? SAPIC?IOAPIC? */
		case ACPI_MADT_TYPE_IO_APIC:
			ioapic_add(nioapic, _.ioapic->Address,
				   _.ioapic->GlobalIrqBase);
			nioapic++;
			break;
		default:
			break;
		}
		len -= *(_.ptr + 1);
		_.ptr += *(_.ptr + 1);
	}

	return 0;
}
#endif				/* MACHINE=i386 */

int acpi_init(void)
{
	ACPI_STATUS r;

	r = AcpiInitializeTables(NULL, 0, 0);
	if (r != AE_OK) {
		printf("ACPICA table init failed.\n");
		return -1;
	}


	r = AcpiGetTable("APIC", 1, (ACPI_TABLE_HEADER **) & acpi_madt);
	if (r != AE_OK) {
		printf("APCICA MADT table retrieve failed.\n");
		return -1;
	}

	return acpi_madt_scan();
}

/* X86 BIOS only.  */
void *acpi_findrootptr(void)
{
	ACPI_STATUS as = 0;
	ACPI_SIZE rptr = 0;

	as = AcpiFindRootPointer(&rptr);

	acpi_rdsp = rptr;
	if (as != AE_OK)
		return NULL;

	printf("ACPI root pointer found at address %p\n", (void *) rptr);
	return (void *) (uintptr_t) rptr;
}
