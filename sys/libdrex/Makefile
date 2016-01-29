SRCROOT=../..
MKDIR=$(SRCROOT)/mk
include $(MKDIR)/mk.conf
include $(MACHINE)/Makefile.inc

LIBNAME=drex
LIBDIR=/lib
SRCS+= vm.c vmap.c lwt.c irq.c kqueue.c

INCS= drex.h event.h irq.h lwt.h preempt.h vmap.h
INCDIR= drex

include $(MKDIR)/inc.mk
include $(MKDIR)/obj.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk
