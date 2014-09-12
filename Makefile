SRCROOT=.
MKDIR=$(SRCROOT)/mk
SUBDIRS+= ukern

include $(MKDIR)/subdir.mk
include $(MKDIR)/def.mk
