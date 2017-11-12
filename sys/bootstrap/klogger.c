#include <microkernel.h>
#include <sys/param.h> /* MIN,MAX */
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <mrg.h>
#include <uk/sys.h>
#include <mrg/consio.h>
#include <vtty/window.h>
#include <vtty/keyb.h>

#include "internal.h"

DEVICE *klogger = NULL;
int klogevt;

/* View. */

WIN *view;
int slines;
int scols;
int viewlines;
int viewcols;
int viewstart;

#define KLOG_LINES 1024

static void
__klog_avl_ast(void *arg __unused)
{
	uint64_t ioval = 0;
	uint8_t ch;

	dout(klogger, IOPORT_BYTE(KLOGDEVIO_ISR), 1);
	din(klogger, IOPORT_DWORD(KLOGDEVIO_SZ), &ioval);
	while (ioval != 0) {
		din(klogger, IOPORT_BYTE(KLOGDEVIO_GETC), &ioval);
		ch = ioval & 0xff;
		vtty_wputc(view, ch);
		din(klogger, IOPORT_DWORD(KLOGDEVIO_SZ), &ioval);
	}
}

void screen_init(void)
{
	WIN *w;
	int viewoff = 1;

	vtty_init(BLACK, YELLOW, XA_NORMAL);
	slines = vtty_lines();
	scols = vtty_cols();
	viewlines = slines - viewoff;
	viewcols = scols;
	viewstart = 0;

	w = vtty_wopen(0, 0, scols- 1, slines - 1, BNONE,
		       XA_NORMAL, BLACK, WHITE, 0, 1);
	vtty_wputs(w, "Kernel Log:");
	vtty_wredraw(w, 1);

	view = vtty_wopen(0, viewoff, viewcols - 1, viewlines - 1,
			  BNONE, XA_NORMAL, BLACK, YELLOW, 0, 1);
	vtty_wcursor(view, CNORMAL);
	vtty_wredraw(view, 1);
}


static void klogger_main(void)
{
	screen_init();

	klogger = dopen("KLOG");
	assert(klogger != NULL);

	/* Enable interrupt */
	klogevt = evtalloc();
	evtast(klogevt, __klog_avl_ast, NULL);
	dmapirq(klogger, KLOGDEVIO_AVLINT, klogevt);
	dout(klogger, IOPORT_BYTE(KLOGDEVIO_IE), 1);
	__klog_avl_ast(NULL);

	lwt_sleep();
}

int
klogger_process(void)
{
	int ret, pid;

	pid = sys_fork();
	if (pid < 0)
		return pid;

	if (pid == 0) {
		klogger_main();
		exit(-1);
	}

	return pid;
}
