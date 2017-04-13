#include <microkernel.h>
#include <sys/param.h> /* MIN,MAX */
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <mrg.h>
#include <mrg/consio.h>
#include <vtty/window.h>

#include "internal.h"

#define CONSOLE_NAMEID squoze("console")
#define KBDBUF_SIZE 16

#define mrg_panic(...) do { printf("UUH"); while(1); } while(0)

static int reqevt = -1;
static unsigned devid = -1;

static int kbdval_req = 0;
static int kbdbuf_fpos = 0;
static int kbdbuf_wpos = 0;
static uint8_t kbdbuf[KBDBUF_SIZE];

static void consdev_putchar(uint8_t v)
{
	console_vga_putchar(v);
}

static void devsts_kbdata_update(int id);

void
kbdbuf_add(int id, uint8_t c)
{
	if (((kbdbuf_wpos + 1) % KBDBUF_SIZE) == kbdbuf_fpos) {
		/* OVERFLOW */
		return;
	}
	kbdbuf[kbdbuf_wpos++] = c;
	kbdbuf_wpos %= KBDBUF_SIZE;

	if (kbdval_req) {
		kbdval_req = 0;
		devsts_kbdata_update(id);
	}
}

static uint64_t
kbdfetch(int id)
{
	int i, len;
	uint64_t val = 0;

	len = kbdbuf_wpos - kbdbuf_fpos;
	if (len == 0)
		return 0;
	if (len < 0)
		len += KBDBUF_SIZE;

	len = MIN(sizeof(uint64_t), len);
	for (i = 0; i < len; i++) {
		val << 8;
		val |= kbdbuf[(kbdbuf_fpos + i) % KBDBUF_SIZE];
	}
	kbdbuf_fpos = (kbdbuf_fpos + len) % KBDBUF_SIZE;

	return val;
}

static void console_io_out(int id, uint32_t port, uint64_t val, uint8_t size);

static void __console_io_ast(void)
{
	uint64_t val = 0;
	int ret;
	struct sys_poll_ior ior;

	while ((ret = devpoll(devid, &ior)) >= 0) {
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
outpos_do(int id, uint64_t val)
{
	unsigned x, y;

	x = val & 0xffff;
	y = val >> 16;
	console_vga_goto(id, x, y);
}

static void
outops_do(int id, uint64_t val)
{
	switch(val) {
	case CONSIO_OUTOP_CURSON:
		console_vga_cursor(id, 1);
		break;
	case CONSIO_OUTOP_CRSOFF:
		console_vga_cursor(id, 0);
		break;
	case CONSIO_OUTOP_SCROLL:
		console_vga_scroll(id);
		break;
	case CONSIO_OUTOP_UPSCRL:
		console_vga_upscroll(id);
		break;
	case CONSIO_OUTOP_BELL:
		console_vga_bell(id);
		break;
	default:
		break;
	}
}

void console_vga_putc(int c, int colattr, int xattr);

static void
outdata_update(int id, uint64_t val)
{
	int c, xattr, colattr;

	c = val & 0xff;
	xattr = (val >> 8) & 0xff;
	colattr = (val >> 16) & 0xff;
	console_vga_putc(c, colattr, xattr);
}

static void
devsts_kbdata_update(int id)
{
	int ret;
	uint64_t val;

	val = kbdfetch(id);
	ret = devwriospace(devid, id, IOPORT_QWORD(CONSIO_KBDATA), val);
	if (ret != 0) {
		printf("error on devwriospace(%d)", id);
		return;
	}

	if (val == 0) {
		kbdval_req = 1;
		return;
	}

	ret = devwriospace(devid, id, IOPORT_BYTE(CONSIO_DEVSTS), CONSIO_DEVSTS_KBDVAL);
	if (ret != 0)
		printf("error on devwriospace(%d)", id);

	ret = devraiseirq(devid, id, CONSIO_IRQ_KBDATA);
	if (ret != 0)
		printf("error raising irq(%d)", id);
}

static void
console_io_out(int id, uint32_t port, uint64_t val, uint8_t size)
{
	devwriospace(devid, id, IOPORT_QWORD(CONSIO_OUTDISP), 80 + (25 << 8));

	switch (port) {
	case CONSIO_DEVSTS:
		if (val & CONSIO_DEVSTS_KBDVAL) {
			devsts_kbdata_update(id);
		}
		break;
	case CONSIO_OUTDATA:
		outdata_update(id, val);
		break;
	case CONSIO_OUTOP:
		outops_do(id, val);
		break;
	case CONSIO_OUTPOS:
		outpos_do(id, val);
		break;
	}
}

static void console_klogon(void)
{
	DEVICE *d;

	d = dopen("SYSTEM");
	assert(d != NULL);
	dout(d, IOPORT_BYTE(SYSDEVIO_CONSON), 1);
	dclose(d);
	d = NULL;
}

static void pltconsole()
{
	int ret;
	struct sys_creat_cfg cfg;
	int i;
	uint64_t pnpid;
	struct device *tmp, *d;
	
	memset(&cfg, 0, sizeof(cfg));
	cfg.nameid = CONSOLE_NAMEID;

	reqevt = evtalloc();
	evtast(reqevt, __console_io_ast);

	ret = devcreat(&cfg, 0111, reqevt);
	if (ret < 0)
		mrg_panic("Cannot create console: devcreat() [%d]", ret);
	devid = ret;

	/* Search for PNP0303 in devices */
	d = NULL;
	pnpid = squoze("PNP0303");
	SLIST_FOREACH(tmp, &devices, list) {
		for (i = 0; i < DEVICEIDS; i++) {
			if (tmp->deviceids[i] == pnpid)  {
				d = tmp;
				break;
			}
		}
	}
	if (d != NULL) {
		console_kbd_init(d->nameid);
	} else {
		printf("No keyboard found");
	}

	/* Search for PNP0900 in devices */
	d = NULL;
	pnpid = squoze("PNP0900");
	SLIST_FOREACH(tmp, &devices, list) {
		for (i = 0; i < DEVICEIDS; i++) {
			if (tmp->deviceids[i] == pnpid)  {
				d = tmp;
				break;
			}
		}
	}
	if (d != NULL) {
		console_klogon();
		console_vga_init(d->nameid);
	} else {
		/* VGA NOT FOUND. Keep using kernel putc */
	}
	lwt_sleep();
}


int
pltconsole_process(void)
{
	int ret, pid;

	pid = sys_fork();
	if (pid < 0)
		return pid;

	if (pid == 0) {
		pltconsole();
		exit(0);
	}

	return pid;
}
