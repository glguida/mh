SRCROOT=../..
MKDIR=$(SRCROOT)/mk
include $(MKDIR)/mk.conf
include $(MACHINE)/Makefile.inc

LIBNAME=mrg
LIBDIR=/lib
SRCS+= vm.c vmap.c lwt.c int.c evt.c dio.c dev.c

INCS= mrg.h mrg_lwt.h mrg_preempt.h
INCDIR= 

include $(MKDIR)/inc.mk
include $(MKDIR)/obj.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk
