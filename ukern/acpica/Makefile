SRCROOT=../../
MKDIR=$(SRCROOT)/mk
OBJDIR=$(SRCROOT)/ukern/acpica/.objdir
ALL_PREDEP = exts
EXTS += acpica

include $(MKDIR)/ukern.mk

LIBNAME=kacpica
NOINST=y

SRCS+= acpi.c acpios.c
VPATH+= $(SRCROOT)/acpica

CFLAGS+= -I $(EXTSDIR)/acpica/include
SRCS+= tbxfroot.c tbxface.c tbxfload.c \
	tbprint.c tbinstal.c tbfind.c \
	tbfadt.c tbutils.c
VPATH+= $(EXTSDIR)/acpica/tables/
SRCS+= utclib.c utxferror.c utexcep.c \
	utresrc.c utglobal.c utalloc.c \
	utmisc.c utstate.c utmutex.c \
	utownerid.c utstring.c utlock.c
VPATH+= $(EXTSDIR)/acpica/utilities/

include $(MKDIR)/obj.mk
include $(MKDIR)/exts.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk

