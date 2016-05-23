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


#define PCI_VENDOR(reg)		((reg) & 0xFFFF)
#define PCI_DEVICE(reg)		(((reg) >> 16) & 0xFFFF)
#define PCI_VENDOR_INVALID	0xFFFF
#define PCI_VENDOR_ID		0x00

#define PCI_HDRTYPE		0x0E

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
		return pltpci_rdcfg_io(bus, dev, func, reg, width, value);
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
		return pltpci_wrcfg_io(bus, dev, func, reg, width, value);
	default:
	case UNKNOWN:
		return -ENOTSUP;
	}
}

static int
getnfuncs(int bus, int dev)
{
	int ret;
	uint32_t hdr;

	ret = pltpci_rdcfg(bus, dev, 0, PCI_HDRTYPE, 8, &hdr);
	if (ret)
		return ret;
	return hdr & 0x80 ? 8 : 1;
}


int
pltpci_init(void)
{
	uint32_t reg;
	int i, nfuncs, ret, bus, dev, ndevs, func;
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
	ndevs = 0;
	for (bus = 0; bus < 256; bus++) {
		for (dev = 0; dev < 32; dev++) {
			nfuncs = getnfuncs(bus, dev);
			for (func = 0; func < nfuncs; func++) {
				if (pltpci_rdcfg(bus, dev, func,
						 PCI_VENDOR_ID, 32, &reg))
					continue;
				if (PCI_VENDOR(reg) == PCI_VENDOR_INVALID ||
				    PCI_VENDOR(reg) == 0)
					continue;
				printf("%02x:%02x.%d\n", bus, dev, func);
				ndevs++;
			}
		}
	}
	printf("Found %d PCI devices/funcs\n", ndevs);
	
	return 0;
}
