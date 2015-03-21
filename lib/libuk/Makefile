SRCROOT=../..
MKDIR=$(SRCROOT)/mk
SUBDIRS_MKINC+= $(MACHINE)
include $(MKDIR)/mk.conf

LIBNAME=uk
LIBDIR=/lib
SRCS+= signals.c syscalls.c

INCS= microkernel.h
INCDIR=

CFLAGS+= -I $(MACHINE)
ASFLAGS+= -I $(MACHINE)

include $(MKDIR)/subdir.mk
include $(MKDIR)/inc.mk
include $(MKDIR)/obj.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk
