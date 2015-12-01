SRCROOT=../..
MKDIR=$(SRCROOT)/mk
include $(SRCROOT)/common/libc/Makefile.inc
include $(MKDIR)/mk.conf

LIBNAME=csys
LIBDIR=/lib
SRCS+= printf.c subr_prf.c

INCS=syslib.h
INCDIR=

CFLAGS+= -I $(MACHINE) -D LIBSA_PRINTF_LONGLONG_SUPPORT
ASFLAGS+= -I $(MACHINE)

include $(MKDIR)/subdir.mk
include $(MKDIR)/inc.mk
include $(MKDIR)/obj.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk
