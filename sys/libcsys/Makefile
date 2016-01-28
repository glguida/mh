SRCROOT=../..
MKDIR=$(SRCROOT)/mk

MK_CONF_NO_RULES_MK_INCLUDE=1
include $(MKDIR)/mk.conf
include $(SRCROOT)/common/libc/Makefile.inc
include $(MKDIR)/rules.mk
include $(MACHINE)/Makefile.inc

LIBNAME=csys
LIBDIR=/lib
SRCS+= assert.c malloc.c printf.c subr_prf.c

INCS=syslib.h
INCDIR=

CFLAGS+= -I $(MACHINE) -D LIBSA_PRINTF_LONGLONG_SUPPORT
ASFLAGS+= -I $(MACHINE)

include $(MKDIR)/subdir.mk
include $(MKDIR)/inc.mk
include $(MKDIR)/obj.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk
