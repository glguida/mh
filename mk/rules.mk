include $(SRCROOT)/ukern.config

EXTSDIR=$(SRCROOT)/exts
EXTSRCDIR=$(SRCROOT)/exts
INSTALLDIR=$(SRCROOT)/dist
INSTALLINCDIR= $(INSTALLDIR)/usr/include

do_all: all

CFLAGS+= -D_DREX_SOURCE -I$(INSTALLDIR)/usr/include \
	-fno-builtin -nostdinc -nostdlib -Wall \
	-D_DREX_MACHINE=$(MACHINE)
ASFLAGS+= -D_DREX_SOURCE -I$(INSTALLDIR)/usr/include \
	-fno-builtin -nostdinc -nostdlib -Wall -D_ASSEMBLER \
	-D_DREX_MACHINE=$(MACHINE)

CFLAGS+= -O2 -fno-strict-aliasing -fno-delete-null-pointer-checks

include $(MKDIR)/$(MACHINE)/rules.mk

ifneq ($(SUBDIRS_MKINC)z, z)
	include $(addsuffix /Makefile.inc, $(SUBDIRS_MKINC))
endif

