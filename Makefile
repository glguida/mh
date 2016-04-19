SRCROOT=.
MKDIR=$(SRCROOT)/mk
SUBDIRS+= include sys/libdrex lib sys ukern

include $(MKDIR)/subdir.mk
include $(MKDIR)/def.mk
