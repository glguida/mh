SRCROOT=../../
include $(SRCROOT)/mk/mk.conf

SRCS+= bootstrap.c plt_acpi.c plt_usb.c console.c console-kbd_.c console-vga.c klogger.c
PROGNAME= bootstrap
PROGDIR=sys
NOINST=yes

LDADD+= -lsquoze -lacpi -lvtty -lahci

#CFLAGS+= -DCONSOLE_DEBUG_BOOT

include $(MAKEDIR)/obj.mk
include $(MAKEDIR)/sysprog.mk
include $(MAKEDIR)/def.mk
