#ifndef __acpica_h
#define __acpica_h

#ifdef ACPICA_DEBUG
#define dbgprintf(...) printf(__VA_ARGS__)
#else
#define dbgprintf(...)
#endif

void *acpi_findrootptr(void);
int acpi_init(void);

#endif
