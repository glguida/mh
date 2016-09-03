#include <microkernel.h>
#include <stdlib.h>
#include <string.h>
#include "internal.h"

#include <stdio.h>
#include <mrg.h>

#include "ehcihw.h"

DEVICE *ehcid;

volatile struct ehcibase *__ehcibase = NULL;
volatile struct ehciopreg *__ehciopreg = NULL;

volatile void *__ehcishared = NULL; /* Page shared between driver and controller */

struct ehcidrv {
#define EHCIDRV_FLAGS_64BIT 1
#define EHCIDRV_FLAGS_PORTINDICATOR 2
	uint8_t flags;
	uint8_t debugport;	/* Debug port number (> 0) */
	uint8_t n_ports;
	uint8_t n_cc; 		/* Number of Companion Controllers (CC) */
	uint64_t cc_portroute;  /* Nibble-array of host-port/CC */
};

static struct ehcidrv drv;
#define device_is_64bit() (drv.flags & EHCIDRV_FLAGS_64BIT)


static int
ehci_init(uint64_t nameid)
{
	int i, ret;
	ssize_t basesz;
	uint64_t base;
	char name[13];

	unsquozelen(nameid, 13, name);
	printf("EHCI device found: %s\n", name);

	ehcid = dopen(name);
	if (ehcid == NULL) {
		printf("Couldn't open device %s.", name);
		return -ENOENT;
	}

	basesz = dgetmemrange(ehcid, 0, &base);
	if (basesz < sizeof(struct ehcibase)) {
		printf("%d: First memory range smaller than EHCI base area.\n", name);
		return -EINVAL;
	}
	__ehcibase = (struct ehcibase *)diomap(ehcid, base, (size_t)basesz); 

	printf("EHCI BASE %"PRIx64":\n", base);
	printf("\tCAPLENGTH: %d\n", __ehcibase->caplength);
	printf("\tHCIVERSION: %d.%d\n",
	       __ehcibase->hciversion >> 8,
	       __ehcibase->hciversion & 0xff);
	printf("\tHCSPARAMS: %x\n", __ehcibase->hcsparams);
	printf("\tHCCPARAMS: %x\n", __ehcibase->hccparams);
	printf("\tHCSP-PORTROUTE: %"PRIx64"\n", __ehcibase->hcsp_portroute);

	memset(&drv, 0, sizeof(drv));
	if (__ehcibase->hccparams & (1 << 0))
		drv.flags |= EHCIDRV_FLAGS_64BIT;
	if (__ehcibase->hcsparams & (1 << 16))
		drv.flags |= EHCIDRV_FLAGS_PORTINDICATOR;
	drv.debugport = (__ehcibase->hcsparams >> 20) & 0xf;
	drv.n_ports = (__ehcibase->hcsparams) & 0xf;
	drv.n_cc = (__ehcibase->hcsparams >> 8) & 0xf;

	/* Setup Routing rules. */
	if (drv.n_cc != 0) {
		switch (__ehcibase->hcsparams & (1 << 7)) {
		case 0: /* N_PCC used. Fill cc_portroute manually */ {
			int i, pcc;
			pcc = (__ehcibase->hcsparams >> 8) & 0xf;
			if (pcc == 0) {
				ret = -EFAULT;
				goto _error;
			}
			for (i = 0; i < drv.n_ports; i++)
				drv.cc_portroute |= (uint64_t)(i % pcc) << (i << 4);
		}
		default:
			drv.cc_portroute = __ehcibase->hcsp_portroute;
		}
	}
	printf("\tCC PORT ROUTE: %016"PRIx64"\n", drv.cc_portroute);

	__ehciopreg = (uint8_t *)__ehcibase + __ehcibase->caplength;
	printf("EHCI device %s: Found %d ports.\n", name, drv.n_ports);
	for (i = 0; i < drv.n_ports; i++) {
		if (__ehciopreg->portsc[i] & 1)
			printf("\t%sPORT%d device present", name, i);
	}

	__ehciopreg->ctrldssegment = 0;
	__ehciopreg->usbintr = 0;
	

	return 0;

_error:
	__ehcibase = NULL;
	dclose(ehcid);
	return ret;
}

static void
pltusb()
{
	int i;
	struct device *d;
	uint64_t cidehci = squoze("PCI0c.03.20");

	SLIST_FOREACH(d, &devices, list) {
		for (i = 0; i < DEVICEIDS; i++) {
			if (d->deviceids[i] == cidehci)  {
				ehci_init(d->nameid);
			}
		}
	}

	_exit(0);
}

int
pltusb_process(void)
{
	int ret, pid;

	/* Search for console devices */
	pid = sys_fork();
	if (pid < 0)
		return pid;

	if (pid == 0)
		pltusb();

	return pid;
}
