SRCROOT=../..
MKDIR=$(SRCROOT)/mk
include $(MKDIR)/mk.conf

LIBNAME=dirtio
LIBDIR=/lib
SRCS+= dirtio.c dirtio_dev.c dirtio_pipe.c dirtio_dev_pipe.c

INCS= dirtio.h dirtio_ring.h dirtio_config.h dirtio_types.h
INCDIR= sys

include $(MKDIR)/inc.mk
include $(MKDIR)/obj.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk
