SRCROOT=../../
MKDIR=$(SRCROOT)/mk

MK_CONF_NO_RULES_MK_INCLUDE=1
include $(MKDIR)/mk.conf
include $(SRCROOT)/common/lib/libc/Makefile.inc
SUBDIRS_MKINC+= gen misc stdlib stdio sys
include $(MKDIR)/rules.mk
include arch/$(MACHINE)/Makefile.inc

LIBNAME=c
LIBDIR=/lib

CFLAGS+= -Iinclude/ -D__LIBC_THREAD_STUBS -D_LIBC -DLIBSA_PRINTF_LONGLONG_SUPPORT -DLIBSA_PRINTF_WIDTH_SUPPORT -DLIBSA_PRINTF_PRECISION_SUPPORT

include $(MKDIR)/subdir.mk
include $(MKDIR)/inc.mk
include $(MKDIR)/obj.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk
