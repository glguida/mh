#include <uk/types.h>
#include "acpica.h"

/* ACPICA includes */
#include "acpi.h"

void *
acpi_findrootptr(void)
{
    ACPI_STATUS as = 0;
    ACPI_SIZE rptr = 0;

    as = AcpiFindRootPointer(&rptr);
    if (as != AE_OK)
	return NULL;
    else
	return (void *)(uintptr_t)rptr;
}
