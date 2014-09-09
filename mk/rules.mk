include $(SRCROOT)/ukern.config

EXTSDIR=$(SRCROOT)/exts
EXTSRCDIR=$(SRCROOT)/exts
INSTALLDIR=$(SRCROOT)/dist
INSTALLINCDIR= $(INSTALLDIR)/usr/include

CFLAGS+= -D_DREX_SOURCE -I$(INSTALLDIR)/usr/include \
	-fno-builtin -nostdinc -nostdlib -Wall \
	-D_DREX_MACHINE=$(MACHINE)
ASFLAGS+= -D_DREX_SOURCE -I$(INSTALLDIR)/usr/include \
	-fno-builtin -nostdinc -nostdlib -Wall -D_ASSEMBLER \
	-D_DREX_MACHINE=$(MACHINE)

CFLAGS+= -O2 -fno-strict-aliasing -fno-delete-null-pointer-checks

ifneq ($(DEBUG)z,z)
CFLAGS+= -D__DEBUG
endif

include $(MKDIR)/$(MACHINE)/rules.mk

do_all: all

ifneq ($(SUBDIRS_MKINC)z, z)
include $(addsuffix /Makefile.inc, $(SUBDIRS_MKINC))
endif

