SRCROOT=../..
MKDIR=$(SRCROOT)/mk
include $(MKDIR)/mk.conf

LIBNAME= sa
LIBDIR= /lib
SRCS+= printf.c subr_prf.c

CFLAGS+= -I $(MACHINE) -D LIBSA_PRINTF_LONGLONG_SUPPORT
ASFLAGS+= -I $(MACHINE)

include $(MKDIR)/subdir.mk
include $(MKDIR)/inc.mk
include $(MKDIR)/obj.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk
