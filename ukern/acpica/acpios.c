#include <uk/types.h>
#include <uk/param.h>

#include "acpi.h"
extern ACPI_PHYSICAL_ADDRESS acpi_rdsp;

void *
AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS pa, ACPI_SIZE len)
{
    return (void *)((uintptr_t)ptova(pa >> PAGE_SHIFT) + (pa&PAGE_MASK));
}

ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer(void)
{

  return acpi_rdsp;
}
