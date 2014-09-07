SRCROOT=../../
MKDIR=$(SRCROOT)/mk
include $(MKDIR)/ukern.mk

LIBNAME=kacpica
NOINST=y
EXTS += acpica

SRCS+= acpi.c
VPATH+= $(SRCROOT)/acpica

CFLAGS+= -I $(EXTSDIR)/acpica/include
SRCS+= tbxfroot.c
VPATH+= $(EXTSDIR)/acpica/tables/


include $(MKDIR)/obj.mk
include $(MKDIR)/exts.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk

