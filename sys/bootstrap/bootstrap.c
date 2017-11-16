#include <stdio.h>
#include <stdlib.h>
#include <microkernel.h>
#include <squoze.h>
#include <assert.h>
#include <sys/param.h>
#include <mrg.h>
#include <mrg/consio.h>
#include <stdlib.h>
#include <machine/param.h>
#include <vtty/window.h>
#include <vtty/keyb.h>
#include "internal.h"

int pltusb_pid = 0;

struct device_list devices = SLIST_HEAD_INITIALIZER(devices);
FILE *syscons;

static int _sys_write(void *cookie, const char *c, int n)
{
	int i;
	for (i = 0; i < n; i++) {
		sys_putc(*c++);
	}
}

static int _console_write(void *cookie, const char *c, int n)
{
	WIN *w = (WIN *)cookie;
	for (int i = 0; i < n; i++) {
		vtty_wputc(w, *c++);
	}
}

void devadd(struct sys_hwcreat_cfg *cfg)
{
	int i, ret;
	struct device *d;

	ret = sys_hwcreat(cfg, 0111);
	if (ret) {
		printf("Error adding device \"%s\": %d\n", unsquoze_inline(cfg->nameid).str, ret);
		return;
	}

	d = malloc(sizeof(*d));
	assert(d != NULL);
	d->nameid = cfg->nameid;
	d->vendorid = cfg->vendorid;
	for (i = 0; i < MIN(DEVICEIDS, SYS_INFO_MAX_DEVIDS); i++)
		d->deviceids[i] = cfg->deviceids[i];
	SLIST_INSERT_HEAD(&devices, d, list);
}

int do_child()
{
	pid_t pid;
	struct sys_childstat cs;

	do {
		pid = sys_childstat(&cs);
		if (pid > 0) {
			if (pid == pltusb_pid) {
				printf("PLTUSB crashed. (%d)\n",
				       cs.exit_status);
			} else
				printf("CHLD %d: exit %d\n", pid,
				       cs.exit_status);
		}
	} while (pid > 0);
}

#include <mrg/blk.h>

int main()
{
	int ret;
	int consevt = -1;
	DEVICE *console;

	syscons = fwopen(NULL, _sys_write);
	stdout = syscons;
	stderr = syscons;
	setvbuf(syscons, NULL, _IONBF, 80);

	printf("MRG bootstrap initiated.\n");
	ret = platform_init();
	if (ret < 0) {
		printf("ACPI platform initialization failed: %d\n", ret);
		return -1;
	}

	/* Become INTRCHILD handler */
	inthandler(INTR_CHILD, do_child, NULL);

	/* Fork root (partition) process */

	/* Wait for either the root process to die (via sigchld, and we panic) or to get a new device */

	/* Run system configuration and inittab */

	/* Become the sigchld handler. Restart services that need to be restarted. */

	/*
	 * THE REST IS HIGHLY TEMPORARY.
	 */


	do {
		int i;
		struct device *d;
		uint64_t cur;
		uint64_t nameid = squoze("PCI01.06.01");
		uint64_t base = squoze("da");

		SLIST_FOREACH(d, &devices, list) {
			for (i = 0; i < DEVICEIDS; i++) {
				if (d->deviceids[i] == nameid)
				  blkdrv_ahci_probe(d->nameid, base);
			}
		}

		for (cur = blk_iter(0); cur; cur = blk_iter(cur)) {
			struct blkdev *blk;
			int evt = evtalloc();
			int evt2 = evtalloc();
			int evt3 = evtalloc();
			int evt4 = evtalloc();
			int evt5 = evtalloc();

			char data[512];
			int r;

			printf("Found disk %s\n", unsquoze_inline(cur).str);
			blk = blk_open(cur);
			blk_read(blk, data, sizeof(data), 0, 1, evt, &r);
			blk_read(blk, data, sizeof(data), 0, 1, evt2, &r);
			blk_read(blk, data, sizeof(data), 0, 1, evt3, &r);
			blk_read(blk, data, sizeof(data), 0, 1, evt4, &r);
			blk_read(blk, data, sizeof(data), 0, 1, evt5, &r);
			evtwait(evt);
			evtclear(evt);
			blk_read(blk, data, sizeof(data), 0, 1, evt, &r);
			evtwait(evt);
			evtclear(evt);
			blk_read(blk, data, sizeof(data), 0, 1, evt, &r);
			evtwait(evt);
			evtclear(evt);
			blk_read(blk, data, sizeof(data), 0, 1, evt, &r);
			evtwait(evt);
			evtclear(evt);
			blk_read(blk, data, sizeof(data), 0, 1, evt, &r);
			evtwait(evt);
			evtclear(evt);
			evtwait(evt2);
			evtwait(evt3);
			evtwait(evt4);
			evtwait(evt5);
			blk_close(blk);
		}
	} while(0);

	//pltusb_pid = pltusb_process();
	/* ret = pltusb_init(); */



	/* fork and start init as configured in sysconfig */


	/* create console device. */
	ret = pltconsole_process();
	if (ret <= 0) {
		printf("console process couldn't start: %d\n", ret);
		return ret;
	}
	printf("PLTCONSOLE started as pid %d\n", ret);

	/* Initialize Window System */
	vtty_init(BLACK, WHITE, XA_NORMAL);
	WIN *ws;
	ws = vtty_wopen(0, 0, vtty_cols() - 1, vtty_lines() - 1,
			BNONE, XA_NORMAL, BLACK, WHITE, 0, 1);
	vtty_wredraw(ws, 1);
	
#ifndef CONSOLE_DEBUG_BOOT
	FILE *consf = fwopen((void *)ws, _console_write);
	setvbuf(consf, NULL, _IONBF, 0);
	stdout = consf;
	stderr = consf;
#endif
	printf("Welcome to the MURGIA system.\n");

	printf("starting kernel log...");
	/* fork and create kloggerd. */
	ret = klogger_process();
	printf("PID %d\n", ret);

	while(1) {
		printf("<%lx>", vtty_kgetcw());
	}
       	lwt_sleep();
}
