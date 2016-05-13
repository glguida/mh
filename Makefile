SRCROOT=.
MKDIR=$(SRCROOT)/mk
SUBDIRS+= include sys/libuk sys/libmrg lib sys ukern

include $(MKDIR)/subdir.mk
include $(MKDIR)/def.mk
