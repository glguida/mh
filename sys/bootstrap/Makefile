SRCROOT=../../
include $(SRCROOT)/mk/mk.conf

SRCS+= bootstrap.c plt_acpi.c plt_usb.c console.c console-kbd_.c console-vga.c
PROGNAME= bootstrap
PROGDIR=sys
NOINST=yes

LDADD+= -lsquoze -lacpi

include $(MAKEDIR)/obj.mk
include $(MAKEDIR)/sysprog.mk
include $(MAKEDIR)/def.mk
