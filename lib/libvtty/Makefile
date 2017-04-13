SRCROOT=../..
MKDIR=$(SRCROOT)/mk
include $(MKDIR)/mk.conf

LIBNAME=vtty
LIBDIR=/usr/lib
SRCS+= window.c vt100.c consio_vtdrv.c

INCS= vt100.h window.h vtdrv.h
INCDIR= vtty

include $(MKDIR)/inc.mk
include $(MKDIR)/obj.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk
