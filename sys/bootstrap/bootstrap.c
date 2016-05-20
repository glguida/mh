#include <stdio.h>
#include <stdlib.h>
#include <microkernel.h>
#include <squoze.h>
#include <assert.h>
#include <sys/param.h>
#include <mrg.h>
#include <mrg/consio.h>
#include "internal.h"

struct device_list devices = SLIST_HEAD_INITIALIZER(devices);

void
devadd(struct sys_hwcreat_cfg *cfg)
{
	int i, ret;
	struct device *d;

	ret = sys_hwcreat(cfg, 0111);
	if (ret) {
		char *name = unsquoze(cfg->nameid);
		printf("Error adding device \"%s\": %d\n", name, ret);
		free(name);
		free(d);
	}

	d = malloc(sizeof(*d));
	assert(d != NULL);
	d->nameid = cfg->nameid;
	d->vendorid = cfg->vendorid;
	for (i = 0; i < MIN(DEVICEIDS,SYS_RDCFG_MAX_DEVIDS); i++)
		     d->deviceids[i] = cfg->deviceids[i];
	SLIST_INSERT_HEAD(&devices, d, list);
}

int
main()
{
	int ret;
	DEVICE *console;

	printf("MRG bootstrap initiated.\n");

	ret = pltacpi_init();
	if (ret < 0) {
		printf("ACPI platform initialization failed: %d\n", ret);
		return -1;
	}

	ret = pltpci_init();
	if (ret < 0)
		printf("PCI bus scan failed: %d\n", ret);

	/* ret = pltusb_init(); */

	/* fork and create console device. */
	ret = pltconsole_process();

	/* fork and create kloggerd.
	 * ret = kloggerd_process(); */

	/* Become SIGCHILD handler */

	/* open console */
	console = dopen("console");
	while (console == NULL) {
		printf("(%d)...", ret);
		lwt_pause();
		console = dopen("console");
	}

	uint64_t val = 0x4141414141414141LL;
	ret = dout(console, CONSIO_OUTDATA, val);
	printf("ret = %d\n", ret);
	if (ret < 0)
		return ret;

	ret = dout(console, IOPORT_QWORD(CONSIO_OUTDATA), val);
	ret = dout(console, IOPORT_BYTE(CONSIO_OUTDATA), '\n');
	ret = dout(console, IOPORT_BYTE(CONSIO_OUTDATA), val);
	ret = dout(console, IOPORT_QWORD(CONSIO_OUTDATA), val);
	ret = dout(console, IOPORT_QWORD(CONSIO_OUTDATA), val);
	ret = dout(console, IOPORT_QWORD(CONSIO_OUTDATA), val);
	ret = dout(console, IOPORT_QWORD(CONSIO_OUTDATA), val);
	ret = dout(console, IOPORT_QWORD(CONSIO_OUTDATA), val);
	sys_wait();

	ret = din(console, 1, &val);
	printf("val = %llx\n", val);

	/* start command loop
	 * ret = pltcommand_setup(); */
	lwt_sleep();
}
