SRCROOT=.
MKDIR=$(SRCROOT)/mk
SUBDIRS+= include sys/libuk sys/libdrex lib sys ukern

include $(MKDIR)/subdir.mk
include $(MKDIR)/def.mk
