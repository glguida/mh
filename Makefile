SRCROOT=.
MKDIR=$(SRCROOT)/mk
SUBDIRS+= include lib sys ukern

include $(MKDIR)/subdir.mk
include $(MKDIR)/def.mk
