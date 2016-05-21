#include "internal.h"
#include <sys/types.h>
#include <microkernel.h>
#include <squoze.h>
#include <stdio.h>
#include <errno.h>
#include <mrg.h>

#define perror(...) printf(__VA_ARGS__)
#warning add perror

/*
 * PCI bus.
 */

static DEVICE *rootdev = NULL;

enum pci_cfg_method {
	UNKNOWN,
	IO,
} pci_cfg_method = UNKNOWN;


int
pltpci_rdcfg_io(unsigned bus, unsigned dev, unsigned func,
		uint16_t reg, int width, uint32_t *value)
{
	int ret;
	uint32_t inaddr = 0xcfc + (reg & 3);
	uint32_t outval = (bus << 16)
		| (dev << 11)
		| (func << 8)
		| (reg & ~0x3)
		| 0x80000000; /* enable bit */
	uint32_t port;
	uint64_t cfgval;

	ret = dout(rootdev, IOPORT_DWORD(0xcf8), outval);
	if (ret)
		return ret;
	switch (width) {
	case 32: 
		port = IOPORT_DWORD(inaddr);
		break;
	case 16:
		port = IOPORT_WORD(inaddr);
		break;
	case 8:
		port = IOPORT_BYTE(inaddr);
		break;
	default:
		return -EINVAL;
	}
	ret = din(rootdev, port, &cfgval);
	if (ret) 
		return ret;

	*value = (uint32_t)cfgval;
	return 0;
}

int
pltpci_wrcfg_io(unsigned bus, unsigned dev, unsigned func,
		uint16_t reg, int width, uint32_t value)
{
	int ret;
	uint32_t inaddr = 0xcfc + (reg & 3);
	uint32_t outval = (bus << 16)
		| (dev << 11)
		| (func << 8)
		| (reg & ~0x3)
		| 0x80000000; /* enable bit */
	uint32_t port;

	ret = dout(rootdev, IOPORT_DWORD(0xcf8), outval);
	if (ret)
		return ret;

	switch (width) {
	case 32: 
		port = IOPORT_DWORD(inaddr);
		break;
	case 16:
		port = IOPORT_WORD(inaddr);
		break;
	case 8:
		port = IOPORT_BYTE(inaddr);
		break;
	default:
		return -EINVAL;
	}
	ret = dout(rootdev, port, (uint64_t)value);
	if (ret)
		return ret;

	return 0;
}

int
pltpci_rdcfg(unsigned bus, unsigned dev, unsigned func,
	     uint16_t reg, int width, uint32_t *value)
{

	switch(pci_cfg_method) {
	case IO:
		return pltpci_rdcfg(bus, dev, func, reg, width, value);
	default:
	case UNKNOWN:
		return -ENOTSUP;
	}
}

int
pltpci_wrcfg(unsigned bus, unsigned dev, unsigned func,
	     uint16_t reg, int width, uint32_t value)
{

	switch(pci_cfg_method) {
	case IO:
		return pltpci_wrcfg(bus, dev, func, reg, width, value);
	default:
	case UNKNOWN:
		return -ENOTSUP;
	}
}

int
pltpci_init(void)
{
	int i, ret;
	char name[13];
	struct dinfo info;
	struct device *tmp, *d = NULL;
	uint64_t pnpid = squoze("PNP0A03");


	/*
	 * Search for PCI root.
	 */

	/* Search for PNP0A03 in devices */
	SLIST_FOREACH(tmp, &devices, list) {
		for (i = 0; i < DEVICEIDS; i++) {
			if (tmp->deviceids[i] == pnpid)  {
				d = tmp;
				break;
			}
		}
	}
	if (d == NULL) {
		printf("PCI root device not found\n");
		return -ESRCH;
	}

	unsquozelen(d->nameid, 13, name);
	printf("PCI root device found at %s\n", name);


	/*
	 * Open PCI device
	 */

	rootdev = dopen(name);
	if (rootdev == NULL) {
		perror("open(%s)", name);
		return -ENOENT;
	}

	ret = dgetinfo(rootdev, &info);
	if (ret < 0) {
		perror("dinfo(%s)", name);
		return -EINVAL;
	}

	/*
	 * Determine PCI access method.
	 */

	/* Having I/O  ports is a  pretty good indication of  us being
	 * able to use I/O ports.  */
	if ((info.npios != 0) && (dgetpio(rootdev, 0) == 0xcf8)) {
		pci_cfg_method = IO;
		goto scan;
	}

	if (pci_cfg_method == UNKNOWN)
		return -ENOTSUP;

	/*
	 * Scan PCI bus
	 */
scan:
	

	return 0;
}
