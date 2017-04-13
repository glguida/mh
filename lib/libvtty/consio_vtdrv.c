#include <sys/cdefs.h>
#include <mrg.h>
#include <mrg/consio.h>
#include "vtdrv.h"

DEVICE *cons;
int cur_attr = 0;
int cur_colattr = 0;
int consevt = -1;

__weak int vtdrv_init(void)
{
	int i;

	cons = dopen("console");
	i = 0;
	while (cons == NULL && i++ < 3){
		lwt_pause();
		cons = dopen("console");
	}
	if (cons == NULL)
		return -1;

	consevt = evtalloc();
	dmapirq(cons, CONSIO_IRQ_KBDATA, consevt);

	return 0;
}

__weak short vtdrv_lines(void)
{
  	uint64_t cfg = 0;
  	din(cons, IOPORT_WORD(CONSIO_OUTDISP), &cfg);
	return (cfg >> 8) & 0xff;
}

__weak short vtdrv_columns(void)
{
	uint64_t cfg = 0;
	din(cons, IOPORT_WORD(CONSIO_OUTDISP), &cfg);
	return cfg & 0xff;
}

__weak void  vtdrv_exit(void)
{
	dclose(cons);
	cons = NULL;
}

__weak void  vtdrv_set_attr(char attr)
{
	cur_attr = attr;
}

#define CONS_OP(_x)dout(cons, IOPORT_BYTE(CONSIO_OUTOP), (CONSIO_OUTOP_##_x))

__weak void  vtdrv_set_color(int colattr)
{
	cur_colattr = colattr;
}

__weak void  vtdrv_cursor(int on)
{
	if (on)
		CONS_OP(CURSON);
	else
		CONS_OP(CRSOFF);
}

__weak void  vtdrv_goto(short x, short y)
{
	uint32_t val = ((uint32_t)y  << 16) + x;
	dout(cons, IOPORT_DWORD(CONSIO_OUTPOS), val);
}

__weak void  vtdrv_scroll(void)
{
	CONS_OP(SCROLL);
}

__weak void  vtdrv_upscroll(void)
{
	CONS_OP(UPSCRL);
}

__weak void  vtdrv_putc(char c)
{
	uint32_t val = 0;

	val = ((uint32_t)cur_colattr << 16) + (cur_attr << 8) + c;
	dout(cons, IOPORT_DWORD(CONSIO_OUTDATA), val);
}

__weak void  vtdrv_bell(void)
{
	CONS_OP(BELL);
}

__weak char vtdrv_getch(void)
{
	uint64_t kbdval, val;

	din(cons, IOPORT_BYTE(CONSIO_DEVSTS), &kbdval);
	if (!(kbdval & CONSIO_DEVSTS_KBDVAL)) {
		dout(cons, IOPORT_BYTE(CONSIO_DEVSTS),
		     CONSIO_DEVSTS_KBDVAL);
		evtwait(consevt);
	}
	din(cons, IOPORT_BYTE(CONSIO_KBDATA), &val);

	return (char)val;
}
