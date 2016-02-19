SRCROOT=../..
MKDIR=$(SRCROOT)/mk
MK_CONF_NO_RULES_MK_INCLUDE=1
include $(MKDIR)/mk.conf
include arch/$(MACHINE)/Makefile.inc
include common/Makefile.inc
include $(MKDIR)/rules.mk

LIBNAME=csu
LIBDIR=/lib

$(OBJDIR)/crt0.o: $(OBJDIR)/crt0.S.o $(OBJDIR)/crt0-common.c.o
	$(LD) $(USRLDFLAGS) -r -o $@ $^

.PHONY: __clean_special_objs
__clean_special_objs:
	-rm $(OBJDIR)/crt0.S.o $(OBJDIR)/crto-common.c.o

EXTOBJS+= $(OBJDIR)/crtbegin.o $(OBJDIR)/crtend.o $(OBJDIR)/crti.o $(OBJDIR)/crtn.o $(OBJDIR)/crt0.o

include $(MKDIR)/obj.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk

