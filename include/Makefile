SRCROOT=..
include $(SRCROOT)/mk/mk.conf

INCS= assert.h inttypes.h stdbool.h stddef.h stdlib.h string.h
INCDIR=

INCSUBDIRS= \
	$(SRCROOT)/ukern/src \
	$(SRCROOT)/ukern/$(MACHINE)/src \
	$(SRCROOT)/sys/sys \
	$(SRCROOT)/sys/$(MACHINE)/include \
	$(SRCROOT)/sys/libcsys

include $(MAKEDIR)/inc.mk
include $(MAKEDIR)/def.mk
