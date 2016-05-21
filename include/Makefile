SRCROOT=..
include $(SRCROOT)/mk/mk.conf

INCS= assert.h errno.h inttypes.h limits.h setjmp.h stdarg.h	\
	stdbool.h stddef.h stdio.h stdlib.h string.h
INCDIR= ./

INCS+= $(SRCROOT)/sys/sys/stdint.h

INCSUBDIRS= \
	ssp \
	$(SRCROOT)/ukern/src \
	$(SRCROOT)/ukern/$(MACHINE)/src \
	$(SRCROOT)/sys/sys \
	$(SRCROOT)/sys/$(MACHINE)/include \
	$(SRCROOT)/sys/libmrg \
	$(SRCROOT)/lib/libsquoze

include $(MAKEDIR)/inc.mk
include $(MAKEDIR)/def.mk
