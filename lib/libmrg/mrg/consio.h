#ifndef __mrg_consio_h
#define __mrg_consio_h

#define CONSIO_DEVSTS 0
#define CONSIO_DEVSTS_KBDVAL (1 << 0)

#define CONSIO_KBDATA 1

#define CONSIO_OUTDATA 32

#define CONSIO_OUTPOS  33

#define CONSIO_OUTOP   35

#define CONSIO_OUTOP_BELL   0
#define CONSIO_OUTOP_SCROLL 1
#define CONSIO_OUTOP_UPSCRL 2
#define CONSIO_OUTOP_CURSON 3
#define CONSIO_OUTOP_CRSOFF 4

#define CONSIO_IRQ_KBDATA 0
#define CONSIO_IRQ_REDRAW 1


/*
 * Special key types returned by CONSIO. The rest are ASCII.
 */
#define K_BS		8
#define K_HT		9
#define K_ENT		10
#define K_ESC		27
#define K_STOP		256
#define K_F1		257
#define K_F2		258
#define K_F3		259
#define K_F4		260
#define K_F5		261
#define K_F6		262
#define K_F7		263
#define K_F8		264
#define K_F9		265
#define K_F10		266

#define K_HOME		267
#define K_PGUP		268
#define K_UP		269
#define K_LT		270
#define K_RT		271
#define K_DN		272
#define K_END		273
#define K_PGDN		274
#define K_INS		275
#define K_DEL		276

#define K_F11		277
#define K_F12		278

#define K_CAP		279
#define K_NUM		280
#define K_SCL		281

#define K_CTL		283
#define K_ALT		284
#define K_SFT		285

#define K_PSCR		286
#define K_BRK		287	
#define K_SYS		288


#define M_CTL		512
#define M_ALT	       1024
#define M_SFT	       2048


#endif
