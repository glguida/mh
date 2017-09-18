SRCROOT=../..
MKDIR=$(SRCROOT)/mk
SUBDIRS_MKINC+= $(MACHINE)
include $(MKDIR)/mk.conf

LIBNAME=uk
LIBDIR=/lib
SRCS+= syscalls.c signals.c vm.c

INCS= microkernel.h
INCDIR=
INCSUBDIRS= $(MACHINE)

CFLAGS+= -I. -I$(MACHINE)
ASFLAGS+= -I. -I$(MACHINE)

include $(MKDIR)/subdir.mk
include $(MKDIR)/inc.mk
include $(MKDIR)/obj.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk
