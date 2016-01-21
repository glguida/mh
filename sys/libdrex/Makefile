SRCROOT=../..
MKDIR=$(SRCROOT)/mk
include $(MKDIR)/mk.conf
include $(MACHINE)/Makefile.inc

LIBNAME=drex
LIBDIR=/lib
SRCS+= dirtio.c vm.c vmap.c lwt.c

INCS= drex.h lwt.h
INCDIR= drex

include $(MKDIR)/inc.mk
include $(MKDIR)/obj.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk
