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

    r = AcpiInitializeTables(NULL, 0, 0);
    if (r != AE_OK) {
	printf("ACPICA table init failed.\n");
	return -1;
    }

    return 0;
}

/* X86 BIOS only.  */
void *
acpi_findrootptr(void)
{
    ACPI_STATUS as = 0;
    ACPI_SIZE rptr = 0;

    as = AcpiFindRootPointer(&rptr);

    acpi_rdsp = rptr;
    if (as != AE_OK)
	return NULL;

    printf("ACPI root pointer found at address %lx\n", rptr);
    return (void *)(uintptr_t)rptr;
}
