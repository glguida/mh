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
		char *name = unsquoze(cfg->nameid);
		printf("Error adding device \"%s\": %d\n", name, ret);
		free(name);
		return;
	}

	d = malloc(sizeof(*d));
	assert(d != NULL);
	d->nameid = cfg->nameid;
	d->vendorid = cfg->vendorid;
	for (i = 0; i < MIN(DEVICEIDS, SYS_RDCFG_MAX_DEVIDS); i++)
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

	pltusb_pid = pltusb_process();
	/* ret = pltusb_init(); */

	/* run sysconfig. Will get system configuration and inittab */

	/* fork and start init as configured in sysconfig */

	/*
	 * THE REST IS HIGHLY TEMPORARY.
	 */

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
	ws = vtty_wopen(0, 0, 79, 24,
			BNONE, XA_NORMAL, BLACK, WHITE, 0, 0, 1);
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
