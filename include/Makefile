SRCROOT=..
include $(SRCROOT)/mk/mk.conf

INCS= assert.h errno.h inttypes.h limits.h setjmp.h stdarg.h	\
	stdbool.h stddef.h stdio.h stdlib.h string.h
INCDIR= ./

INCS+= sys/stdint.h

INCSUBDIRS= \
	ssp \
	sys \
	$(MACHINE) \
	$(SRCROOT)/ukern/src \
	$(SRCROOT)/ukern/$(MACHINE)/src \
	$(SRCROOT)/lib/libmrg \
	$(SRCROOT)/lib/libsquoze

include $(MAKEDIR)/inc.mk
include $(MAKEDIR)/def.mk
