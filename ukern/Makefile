SRCROOT= ..
include $(SRCROOT)/ukern.config
include $(MACHINE)/Makefile.inc
SUBDIRS_MKINC+= lib/ ukern/
SUBDIRS+= include acpica

MKDIR=$(SRCROOT)/mk
include $(MKDIR)/ukern.mk

include $(MKDIR)/exts.mk
include $(MKDIR)/obj.mk
include $(MKDIR)/subdir.mk

LIBS+= -lkacpica
LDFLAGS+= -Lacpica


ukern: $(OBJS)
	$(LD) $(LDFLAGS) -o $(INSTALLDIR)/$@ $(OBJS) $(LIBS)

ALL_TARGET+= ukern

all: $(ALL_TARGET)

clean: $(CLEAN_TARGET)
	-rm $(INSTALLDIR)/ukern $(OBJS)
