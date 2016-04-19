#include <uk/types.h>
#include <lib/lib.h>
#include "i386.h"

#define PCI_CFG_STS_INTS (1 << 3)

static uint32_t pci_cfg_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t r)
{
	uint32_t addr = (bus << 16) | (dev << 11) | (func << 8) | (r << 2);

	/* enable bit */
	addr |= 0x80000000;

	outl(PCI_CFG_ADDR, addr);
	return inl(PCI_CFG_DATA);
}

int pci_cfg_intsts(uint8_t bus, uint8_t dev, uint8_t fn)
{
	uint16_t status;
	uint32_t stscmd;

	stscmd = pci_cfg_read(bus, dev, fn, 1);
	if (stscmd == (uint32_t)-1) {
		printf("Non existing PCI device: [%02d:%02d.%d]\n",
		       bus, dev, fn);
		return 0;
	}
	status = (uint16_t)(stscmd >> 16);
	return !!(status & PCI_CFG_STS_INTS);
}
