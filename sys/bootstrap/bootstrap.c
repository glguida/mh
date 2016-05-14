#include <stdio.h>
#include <stdlib.h>
#include <microkernel.h>
#include <squoze.h>
#include <assert.h>
#include <sys/param.h>
#include <mrg.h>
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

int run_lwt(void *arg)
{
	printf("Running!\n");
	preempt_disable();
	__evtset((int)(uintptr_t)arg);
	preempt_enable();
	lwt_exit();
}

int main()
{
	int ret;

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

	/* fork and create console device.
	 * ret = pltconsole_process(); */

	/* fork and create kloggerd.
	 * ret = kloggerd_process(); */

	/* Become SIGCHILD handler */

	/* open console and start command loop.
	 * ret = pltcommand_setup(); */

#if 1
{
	int evt;

	lwt_t *new = lwt_create(run_lwt, (void *)3, 1024);
	lwt_wake(new);
	printf("Created lwt %p\n", new);

	evt = evtalloc();
	printf("allocated evt %d\n", evt);
	evt = evtalloc();
	printf("allocated evt %d\n", evt);
	evt = evtalloc();
	printf("allocated evt %d\n", evt);
	evt = evtalloc();
	printf("allocated evt %d\n", evt);

	evtwait(evt);
	printf("Done!\n");

	dopen("PCI0", 0111);
}
#endif

	lwt_sleep();
}
