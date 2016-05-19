SRCROOT=.
MKDIR=$(SRCROOT)/mk
SUBDIRS+= include sys/libuk sys/libmrg lib sys

ifneq ($(TESTS)z,z)
SUBDIRS += tests
endif

SUBDIRS+= ukern

include $(MKDIR)/subdir.mk
include $(MKDIR)/def.mk
