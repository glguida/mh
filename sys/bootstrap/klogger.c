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
DEVICE *console;
int klogevt;

static void console_putchar(int ch)
{
	uint32_t val;
	val = ch | (XA_NORMAL << 8) | (COLATTR(RED,BLUE) << 16);
	dout(console, IOPORT_DWORD(CONSIO_OUTDATA), val);
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

	console = dopen("console");

	int i = 0;
	while (console == NULL && i++ < 3) {
		lwt_pause();
		console = dopen("console");
	}
	if (console == NULL) {
		printf("klogger: error opening console!\n");
		exit(-1);
	}
	
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

	if (pid == 0)
		klogger_main();

	return pid;
}
