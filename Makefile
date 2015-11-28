SRCROOT=.
MKDIR=$(SRCROOT)/mk
SUBDIRS+= include sys ukern

include $(MKDIR)/subdir.mk
include $(MKDIR)/def.mk
