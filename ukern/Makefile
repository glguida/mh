SRCROOT= ..

MKDIR= $(SRCROOT)/mk
MK_CONF_NO_RULES_MK_INCLUDE=1
include $(MKDIR)/mk.conf
include $(SRCROOT)/common/lib/libc/Makefile.inc
OBJDIR=$(SRCROOT)/ukern/.objs

include ukern.config
include $(MACHINE)/Makefile.inc
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


ukern: $(OBJS)
	$(OBJCOPY) -I elf32-i386 -O elf32-i386 \
		--remove-section .init.elf \
		--add-section .init.elf=$(INSTALLDIR)/sys/bootstrap \
		--set-section-flags .init.elf=alloc,load,data \
		$(OBJDIR)/empty.o 
	$(LD) $(LDFLAGS) $(KERNLDFLAGS) -o $(INSTALLDIR)/$@ $(OBJS) $(LIBS) $(LDADD)


ALL_TARGET+= ukern

all: $(ALL_TARGET)

clean: $(CLEAN_TARGET)
	-rm $(INSTALLDIR)/ukern $(OBJS)
