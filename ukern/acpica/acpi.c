#include <uk/types.h>
#include "acpica.h"

/* ACPICA includes */
#include "acpi.h"

#define NTBLARY 128
ACPI_TABLE_DESC itblarray[NTBLARY];
ACPI_PHYSICAL_ADDRESS acpi_rdsp;

int
acpi_init(void)
{
    ACPI_STATUS r;

    r = AcpiInitializeTables(itblarray, NTBLARY, 0);
    if (r == AE_OK)
	return 0;
    else
	return -1;
}

void *
acpi_findrootptr(void)
{
    ACPI_STATUS as = 0;
    ACPI_SIZE rptr = 0;

    as = AcpiFindRootPointer(&rptr);

    acpi_rdsp = rptr;
    if (as != AE_OK)
	return NULL;
    else
	return (void *)(uintptr_t)rptr;
}
