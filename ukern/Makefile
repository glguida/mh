SRCROOT= ..

MKDIR= $(SRCROOT)/mk
MK_CONF_NO_RULES_MK_INCLUDE=1
include $(MKDIR)/mk.conf
OBJDIR=$(SRCROOT)/ukern/.objs

include ukern.config
include $(MACHINE)/Makefile.inc
include lib/Makefile.inc
SUBDIRS_MKINC+= lib/ src/
SUBDIRS+= src lib acpica
include $(MAKEDIR)/ukern.mk

include $(MAKEDIR)/inc.mk
include $(MAKEDIR)/ukinc.mk
include $(MAKEDIR)/exts.mk
include $(MAKEDIR)/obj.mk
include $(MAKEDIR)/subdir.mk

LIBS+= -lkacpica
LDFLAGS+= -Lacpica

ifneq ($(TESTS)z,z)
INIT_BINARY= $(SRCROOT)/tests/test
else
INIT_BINARY=$(INSTALLDIR)/sys/bootstrap
endif


ukern: $(OBJS)
	$(CP) $(INIT_BINARY) $(INIT_BINARY).sym
	$(STRIP) -s $(INIT_BINARY)
	$(OBJCOPY) -I elf32-i386 -O elf32-i386 \
		--remove-section .init.elf \
		--add-section .init.elf=$(INIT_BINARY) \
		--set-section-flags .init.elf=alloc,load,data \
		$(OBJDIR)/empty.o 
	$(LD) $(LDFLAGS) $(KERNLDFLAGS) -o $(INSTALLDIR)/$@ $(OBJS) $(LIBS) $(LDADD)
	$(CP) $(INSTALLDIR)/$@ $(INSTALLDIR)/$@.sym
	$(STRIP) -s $(INSTALLDIR)/$@


ALL_TARGET+= ukern

all: $(ALL_TARGET)

clean: $(CLEAN_TARGET)
	-rm $(INSTALLDIR)/ukern $(OBJS)
