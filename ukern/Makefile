SRCROOT= ..
include $(SRCROOT)/mk/mk.conf
OBJDIR=$(SRCROOT)/ukern/.objs
OBJCOPY?=objcopy

include ukern.config
include $(MACHINE)/Makefile.inc
SUBDIRS_MKINC+= lib/ ukern/
SUBDIRS+= include acpica
include $(MAKEDIR)/ukern.mk

include $(MAKEDIR)/exts.mk
include $(MAKEDIR)/obj.mk
include $(MAKEDIR)/subdir.mk

LIBS+= -lkacpica
LDFLAGS+= -Lacpica


ukern: $(OBJS)
	$(OBJCOPY) -I binary /dev/null -O elf32-i386 $(OBJDIR)/init.elf.o
	$(OBJCOPY) -I elf32-i386 -B i386 \
		--add-section .init.elf=$(INSTALLDIR)/sys/init \
		--set-section-flags .init.elf=alloc,load,data \
		 $(OBJDIR)/init.elf.o 
	$(LD) $(LDFLAGS) -o $(INSTALLDIR)/$@ $(OBJS) $(OBJDIR)/init.elf.o $(LIBS)

ALL_TARGET+= ukern

all: $(ALL_TARGET)

clean: $(CLEAN_TARGET)
	-rm $(INSTALLDIR)/ukern $(OBJS) $(OBJDIR)/init.elf.o
