SRCROOT=../..
MKDIR=$(SRCROOT)/mk

MK_CONF_NO_RULES_MK_INCLUDE=1
include $(MKDIR)/mk.conf
include $(MKDIR)/rules.mk
include $(MACHINE)/Makefile.inc

LIBNAME=mrg
LIBDIR=/lib
SRCS+= vm.c vmap.c dma32.c lwt.c int.c evt.c dio.c dev.c blk.c
SRCS+= stdc.c

INCSUBDIRS= mrg $(MACHINE)/include
INCS= mrg.h mrg_lwt.h mrg_preempt.h
INCDIR= 

include $(MKDIR)/inc.mk
include $(MKDIR)/obj.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk
