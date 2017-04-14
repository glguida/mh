#include <microkernel.h>
#include <sys/param.h> /* MIN,MAX */
#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <mrg.h>
#include <uk/sys.h>
#include <mrg/consio.h>
#include <vtty/window.h>

#include "internal.h"

DEVICE *klogger = NULL;
int klogevt;
WIN *w;


static void console_putchar(int ch)
{
	vtty_wputc(w, ch);
}

static void
__klog_avl_ast()
{
	uint64_t ioval = 0;
	uint8_t ch;

	dout(klogger, IOPORT_BYTE(KLOGDEVIO_ISR), 1);
	din(klogger, IOPORT_DWORD(KLOGDEVIO_SZ), &ioval);
	while (ioval != 0) {
		din(klogger, IOPORT_BYTE(KLOGDEVIO_GETC), &ioval);
		ch = ioval & 0xff;
		console_putchar(ch);
		din(klogger, IOPORT_DWORD(KLOGDEVIO_SZ), &ioval);
	}
}

static void klogger_main(void)
{
	klogger = dopen("KLOG");
	assert(klogger != NULL);

	win_init(BLACK, YELLOW, XA_NORMAL);
	w = vtty_wopen(0, 0, 79, 24, BNONE, XA_NORMAL, BLACK, WHITE, 0, 0, 1);
	vtty_wredraw(w, 1);
	vtty_wputs(w, "Kernel Log:");
	w = vtty_wopen(0, 1, 79, 24, BNONE, XA_NORMAL, BLACK, YELLOW, 0, 0, 1);


	vtty_wcursor(w, CNONE);
	vtty_wredraw(w, 1);

	/* Enable interrupt */
	klogevt = evtalloc();
	evtast(klogevt, __klog_avl_ast);
	dmapirq(klogger, KLOGDEVIO_AVLINT, klogevt);
	dout(klogger, IOPORT_BYTE(KLOGDEVIO_IE), 1);
	__klog_avl_ast();
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
