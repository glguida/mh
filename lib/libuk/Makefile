SRCROOT=../..
MKDIR=$(SRCROOT)/mk
include $(MKDIR)/mk.conf

LIBNAME=uk
LIBDIR=/lib
SRCS= syscalls.c

INCS= microkernel.h
INCDIR=

CFLAGS+= -I $(MACHINE)
ASFLAGS+= -I $(MACHINE)

include $(MKDIR)/inc.mk
include $(MKDIR)/obj.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk
