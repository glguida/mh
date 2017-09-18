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
#define CONSOLE_VENDORID squoze("mrgsys")
#define CONSOLE_MAXSCREENS 256

static int reqevt = -1;
static unsigned devid = -1;

struct screen {
	int active;

	/* Keyboard I/O State. */
	int req;
	int fpos;
	int wpos;
#define KBDBUF_SIZE 16
	uint16_t kbdbuf[KBDBUF_SIZE];
	void *kbd_opq;

	/* Display I/O State. */
	void *dsp_opq;
};

unsigned cur_screen = 0;
struct screen screens[CONSOLE_MAXSCREENS];
#define cscr() (screens + cur_screen)
#define scr(_id) (screens + (_id))

static void devsts_kbdata_update(int id);

void
kbdbuf_add(uint16_t c)
{
	int fpos, wpos;
	struct screen *s = cscr();
	
	if (((s->wpos + 1) % KBDBUF_SIZE) == s->fpos) {
		/* OVERFLOW */
		return;
	}
	s->kbdbuf[s->wpos++] = c;
	s->wpos %= KBDBUF_SIZE;

	if (s->req) {
		s->req = 0;
		devsts_kbdata_update(cur_screen);
	}
}

static uint16_t
kbdfetch(int id)
{
	struct screen *s= scr(id);
	uint64_t val = 0;
	int i, len;

	len = s->wpos - s->fpos;
	if (len == 0)
		return 0;
	if (len < 0)
		len += KBDBUF_SIZE;

	val = s->kbdbuf[s->fpos % KBDBUF_SIZE];
	s->fpos = (s->fpos + 1) % KBDBUF_SIZE;

	return val;
}

static void console_io_open(int id);
static void console_io_clone(int id, int clid);
static void console_io_out(int id, uint32_t port, uint64_t val, uint8_t size);
static void console_io_close(int id);

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
		case SYS_POLL_OP_CLOSE:
			console_io_close(ret);
			break;
		case SYS_POLL_OP_OPEN:
			console_io_open(ret);
			break;
		case SYS_POLL_OP_CLONE:
			console_io_clone(ret, ior.clone_id);
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
	struct screen *s = scr(id);
	unsigned x, y;

	x = val & 0xffff;
	y = val >> 16;
	
	if (s == cscr())
		console_vga_goto(x, y);
}

static void
outops_do(int id, uint64_t val)
{
	struct screen *s = scr(id);

	switch(val) {
	case CONSIO_OUTOP_CURSON:
		if (s == cscr())
			console_vga_cursor(1);
		break;
	case CONSIO_OUTOP_CRSOFF:
		if (s == cscr())
			console_vga_cursor(0);
		break;
	case CONSIO_OUTOP_SCROLL:
		if (s == cscr())
			console_vga_scroll();
		break;
	case CONSIO_OUTOP_UPSCRL:
		if (s == cscr())
			console_vga_upscroll();
		break;
	case CONSIO_OUTOP_BELL:
		/* Ring the bell for all screens */
		console_vga_bell();
		break;
	default:
		break;
	}
}

void console_vga_putc(int c, int colattr, int xattr);

static void
outdata_update(int id, uint64_t val)
{
#ifndef CONSOLE_DEBUG_BOOT
	int c, xattr, colattr;

	if (id != cur_screen)
		return;

	c = val & 0xff;
	xattr = (val >> 8) & 0xff;
	colattr = (val >> 16) & 0xff;
	console_vga_putc(c, colattr, xattr);
#endif
}

static void
devsts_kbdata_update(int id)
{
	int ret;
	uint64_t val;
	struct screen *s = scr(id);

	val = kbdfetch(id);
	ret = devwriospace(devid, id, IOPORT_WORD(CONSIO_KBDATA), val);
	if (ret != 0) {
		printf("error on devwriospace(%d)", id);
		return;
	}
	if (val == 0) {
		s->req = 1;
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
	struct screen *s = scr(id);

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

static void
console_io_close(int id)
{
	struct screen *s = scr(id);

	console_kbd_close(s->kbd_opq);
	console_vga_close(s->dsp_opq);

	s->active = 0;
	s->req = 0;
	s->fpos = 0;
	s->wpos = 0;

	s->kbd_opq = NULL;
	s->dsp_opq = NULL;
}

static void
console_io_open(int id)
{
	struct screen *s = scr(id);

	s->req = 0;
	s->fpos = 0;
	s->wpos = 0;

	s->kbd_opq = NULL;
	s->dsp_opq = NULL;
	s->active = 1;
}

static void
console_io_clone(int id, int clid)
{
	struct screen *s = scr(id);
	struct screen *cls = scr(clid);

	cls->req = s->req;
	cls->fpos = s->fpos;
	cls->wpos = s->fpos;

	/* FIXME */
	cls->kbd_opq = NULL;
	cls->dsp_opq = NULL;
	cls->active = 1;
}

#ifndef CONSOLE_DEBUG_BOOT
static void console_klogon(void)
{
	DEVICE *d;

	d = dopen("SYSTEM");
	assert(d != NULL);
	dout(d, IOPORT_BYTE(SYSDEVIO_CONSON), 1);
	dclose(d);
	d = NULL;
}
#endif

void console_switch(unsigned id)
{
	struct screen *cs = cscr();
	struct screen *ns = scr(id);

	assert(cur_screen < CONSOLE_MAXSCREENS);

	if (id >= CONSOLE_MAXSCREENS)
		return;

	if (!ns->active)
		return;
	
	if (cs != ns) {
		cs->dsp_opq = console_vga_save();
		cs->kbd_opq = console_kbd_save();


		console_vga_goto(0,0);
		console_vga_putc('0', COLATTR(RED,WHITE), XA_BOLD);

		
		console_vga_restore(ns->dsp_opq);
		ns->dsp_opq = NULL;
		console_kbd_restore(ns->kbd_opq);
		ns->kbd_opq = NULL;
		cur_screen = id;
	}
	devraiseirq(devid, id, CONSIO_IRQ_REDRAW);
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
	cfg.vendorid = CONSOLE_VENDORID;
	cfg.usercfg[0] = console_vga_cols() + (console_vga_lines() << 8);

	reqevt = evtalloc();
	evtast(reqevt, __console_io_ast);

	ret = devcreat(&cfg, 0111, reqevt);
	if (ret < 0) {
		exit(-7);
	}
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
#ifndef CONSOLE_DEBUG_BOOT
		console_klogon();
#endif
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
