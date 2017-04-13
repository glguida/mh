#ifndef __mrg_consio_h
#define __mrg_consio_h

#define CONSIO_DEVSTS 0
#define CONSIO_DEVSTS_KBDVAL (1 << 0)

#define CONSIO_KBDATA 1

#define CONSIO_OUTDATA 32
#define CONSIO_OUTPOS  33
#define CONSIO_OUTDISP 34
#define CONSIO_OUTOP   35

#define CONSIO_OUTOP_BELL   0
#define CONSIO_OUTOP_SCROLL 1
#define CONSIO_OUTOP_UPSCRL 2
#define CONSIO_OUTOP_CURSON 3
#define CONSIO_OUTOP_CRSOFF 4

#define CONSIO_IRQ_KBDATA 0

#endif
