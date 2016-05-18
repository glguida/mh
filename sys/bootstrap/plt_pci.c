#include "internal.h"
#include <sys/types.h>
#include <microkernel.h>
#include <squoze.h>
#include <stdio.h>
#include <errno.h>

#define perror(...) printf(__VA_ARGS__)
#warning add perror

/*
 * PCI bus.
 */

static int rootfd = -1;

enum pci_cfg_method {
	UNKNOWN,
	IO,
} pci_cfg_method = UNKNOWN;

#if 0
uint32_t
pltpci_rdcfg_io(unsigned bus, unsigned dev, unsigned func, uint16_t reg)
{
	uint32_t val;
	unsigned addr = 0xc000 | dev << 8 | reg;

	sys_out(rootfd, IOPORT_DWORD(0xcf8), 0xf0 | (func << 1));
	sys_out(rootfd, IOPORT_DWORD(0xcfa), 0x
	outb(bus, 0xcfa);
	val = inl(addr);
	outb(0, 0xcf8);

	return val;
}

void
pltpci_wrcfg_io(unsigned bus, unsigned dev, unsigned func, uint16_t reg,
		uint32_t val)
{
	
}
#endif

int
pltpci_init(void)
{
	int i, ret;
	char name[13];
	struct sys_rdcfg_cfg rootcfg;
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

	rootfd = sys_open(d->nameid);
	if (rootfd < 0) {
		perror("open(%s)", unsquozelen(d->nameid, 13, name));
		return rootfd;
	}
	ret = sys_rdcfg(rootfd, &rootcfg);
	if (ret < 0) {
		printf("?");
		perror("rdcfg(%d)", rootfd);
		return ret;
	}

	/*
	 * Determine PCI access method.
	 */

	rootfd = sys_open(d->nameid);

	/* Having I/O  ports is a  pretty good indication of  us being
	 * able to use I/O ports.  */
	if (rootcfg.npiosegs != 0 && SYS_RDCFG_IOSEG(&rootcfg, 0).base == 0xcf8) {
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
