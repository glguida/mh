SRCROOT=../../
MKDIR=$(SRCROOT)/mk
include $(MKDIR)/ukern.mk

LIBNAME=kacpica
NOINST=y
EXTS += acpica

SRCS+= acpi.c acpios.c
VPATH+= $(SRCROOT)/acpica

CFLAGS+= -I $(EXTSDIR)/acpica/include
SRCS+= tbxfroot.c tbxface.c tbprint.c
VPATH+= $(EXTSDIR)/acpica/tables/
SRCS+= utclib.c utxferror.c utexcep.c
VPATH+= $(EXTSDIR)/acpica/utilities/


include $(MKDIR)/obj.mk
include $(MKDIR)/exts.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk

