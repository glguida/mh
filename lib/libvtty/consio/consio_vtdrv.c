#include <sys/cdefs.h>
#include <mrg.h>
#include <mrg/consio.h>
#include "vtdrv.h"
#include "window.h"

DEVICE *cons;
int cur_attr = 0;
int cur_colattr = 0;
int drawevt = -1;
int keybevt = -1;
int keybon = 1;
int clistevt = -1;

static void __redraw(void)
{
	vtdrv_goto(0,0);
	vtdrv_putc('1');
	vtty_wrestore();
}

#if 0
#include <mrg_keyb.h>
void printchar(int c)
{
	if (c & M_CTL)
		printf("C-");
	c &= ~M_CTL;
	if (c & M_SFT)
		printf("S-");
	c &= ~M_SFT;
	if (c & M_ALT)
		printf("A-");
	c &= ~M_ALT;

	if (c>= 32 && c < 256) {
		printf("%c", c);
	} else {
		char *s;
		switch(c) {
		case K_BS:
			s = "BACKSPACE";
			break;
		case K_HT:
			s = "TAB";
			break;
		case K_ENT:
			s = "ENTER";
			break;
		case K_ESC:
			s = "ESC";
			break;
		case K_STOP:
			s = "STOP";
			break;
		case K_HOME:
			s = "HOME";
			break;
		case K_PGUP:
			s = "PGUP";
			break;
		case K_PGDN:
			s = "PGDOWN";
			break;
		default:
			s = "?";
			break;
		}
		printf("%s ", s);
	}
}
#endif

void _clist_add(int c);
static void __lwt_vttykbd(void *a)
{
	uint64_t val;

	while (keybon) {
		/* Request data */
		dout(cons, IOPORT_BYTE(CONSIO_DEVSTS),
		     CONSIO_DEVSTS_KBDVAL);
		evtwait(keybevt);
		din(cons, IOPORT_WORD(CONSIO_KBDATA), &val);
		evtset(clistevt);
		_clist_add((int)val & 0xffff);
		evtclear(keybevt);
	}
}

int vtdrv_init(void)
{
	int i;
	lwt_t *kblwt;

	cons = dopen("console");
	i = 0;
	while (cons == NULL && i++ < 3){
		lwt_pause();
		cons = dopen("console");
	}
	if (cons == NULL)
		return -1;

	keybevt = evtalloc();
	dmapirq(cons, CONSIO_IRQ_KBDATA, keybevt);

	drawevt = evtalloc();
	evtast(drawevt, __redraw);
	dmapirq(cons, CONSIO_IRQ_REDRAW, drawevt);

	clistevt = evtalloc();
	kblwt = lwt_create(__lwt_vttykbd, NULL, 65536);
	preempt_disable();
	__lwt_wake(kblwt);
	preempt_enable();
	return 0;
}

short vtdrv_lines(void)
{
  	uint64_t val = 0;

	val = dusercfg(cons, 0);
	return (val >> 8) & 0xff;
}

short vtdrv_columns(void)
{
    	uint64_t val = 0;

	val = dusercfg(cons, 0);
	return val & 0xff;
}

void  vtdrv_exit(void)
{
	dclose(cons);
	cons = NULL;
}

void  vtdrv_set_attr(char attr)
{
	cur_attr = attr;
}

#define CONS_OP(_x)dout(cons, IOPORT_BYTE(CONSIO_OUTOP), (CONSIO_OUTOP_##_x))

void  vtdrv_set_color(int colattr)
{
	cur_colattr = colattr;
}

void  vtdrv_cursor(int on)
{
	if (on)
		CONS_OP(CURSON);
	else
		CONS_OP(CRSOFF);
}

void  vtdrv_goto(short x, short y)
{
	uint64_t val = ((uint32_t)y  << 16) + x;

	dout(cons, IOPORT_DWORD(CONSIO_OUTPOS), val);
}

void  vtdrv_scroll(void)
{

	CONS_OP(SCROLL);
}

void  vtdrv_upscroll(void)
{

	CONS_OP(UPSCRL);
}

void  vtdrv_putc(char c)
{
	uint64_t val = 0;

	val = ((uint32_t)cur_colattr << 16) + (cur_attr << 8) + c;
	dout(cons, IOPORT_DWORD(CONSIO_OUTDATA), val);
}

void  vtdrv_bell(void)
{

	CONS_OP(BELL);
}

void vtdrv_kwait(void)
{

	evtwait(clistevt);
	evtclear(clistevt);
}
