#include <microkernel.h>
#include <sys/param.h> /* MIN,MAX */
#include <stdlib.h>

#include <stdio.h>
#include <mrg.h>
#include <mrg/consio.h>

#define CONSOLE_NAMEID squoze("console")

#define mrg_panic(...) do { printf("UUH"); while(1); } while(0)

static int reqevt = -1;

static void consdev_putchar(uint8_t v)
{
	printf("%c", v);
}

static uint64_t
kbdfetch(int id)
{
	return 0;
}

static void console_io_out(int id, uint32_t port, uint64_t val, uint8_t size);

static void __console_io_ast(void)
{
	uint64_t val = 0;
	int ret;
	struct sys_poll_ior ior;

	while ((ret = devpoll(&ior)) >= 0) {
		switch (ior.op) {
		case SYS_POLL_OP_OUT:
			console_io_out(ret, ior.port, ior.val, ior.size);
			break;
		default:
			break;
		}
	}
	evtclear(reqevt);
}

static void
outdata_update(int id, uint64_t val, uint8_t size)
{
	int i, len;
	uint8_t *c = (uint8_t *)&val;

	len = MIN(sizeof(val), (1 << size));
	for (i = 0; i < len; i++, c++)
		consdev_putchar(*c);
}

static void
devsts_kbdata_update(int id)
{
	int ret;
	uint64_t val;

	val = kbdfetch(id);
	if (val == 0) {
		devwriospace(id, CONSIO_KBDATA, 0);
		return;
	}

	ret = devwriospace(id, CONSIO_KBDATA, val);
	if (ret != 0) {
		printf("error on devwriospace(%d)", id);
		return ret;
	}

	ret = devwriospace(id, CONSIO_DEVSTS, CONSIO_DEVSTS_KBDVAL);
	if (ret != 0)
		printf("error on devwriospace(%d)", id);

	ret = devraiseirq(id, 0);
	if (ret != 0)
		printf("error raising irq(%d)", id);
}

static void
console_io_out(int id, uint32_t port, uint64_t val, uint8_t size)
{

	switch (port) {
	case CONSIO_DEVSTS:
		if (val & CONSIO_DEVSTS_KBDVAL) {
			devsts_kbdata_update(id);
		}
		break;
	case CONSIO_OUTDATA:
		outdata_update(id, val, size);
	}
}

static void pltconsole()
{
	int ret;
	struct sys_creat_cfg cfg;
	
	memset(&cfg, 0, sizeof(cfg));
	cfg.nameid = CONSOLE_NAMEID;

	reqevt = evtalloc();
	evtast(reqevt, __console_io_ast);

	ret = devcreat(&cfg, 0111, reqevt);
	if (ret < 0)
		mrg_panic("Cannot create console: devcreat() [%d]", ret);

	lwt_sleep();
}


int
pltconsole_process(void)
{
	int ret, pid;

	/* Search for console devices */
	pid = sys_fork();
	if (pid < 0)
		return pid;

	if (pid == 0) {
		pltconsole();
	} else {
		printf("Created console process with pid %d.", pid);
	}
	return pid;
}
