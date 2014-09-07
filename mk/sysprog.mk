.PHONY: clean_$(PROGNAME) install_$(PROGNAME)
ALL_TARGET+= $(PROGNAME)
CLEAN_TARGET+= clean_$(PROGNAME)
INSTALL_TARGET+= install_$(PROGNAME)

# Standard programs options
CPPFLAGS+= -isystem $(INSTALLDIR)/usr/include
CFLAGS+= -T $(SRCROOT)/make/elf_i386.ld
ASFLAGS+= -isystem $(INSTALLDIR)/usr/include
LDFLAGS+= -B $(INSTALLDIR)/lib -B $(INSTALLDIR)/usr/lib

# System programs specific flags
LDFLAGS+= -L $(INSTALLDIR)/lib/sys
CRT0= $(INSTALLDIR)/lib/sys/crt0.o
CRTEND=
LDADD+= -lcsu

$(PROGNAME): $(OBJS)
	$(CC) -o $(PROGNAME) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(CRT0) $(OBJS) $(LDADD) $(CRTEND)

clean_$(PROGNAME):
	-rm $(OBJS) $(PROGNAME)

install_$(PROGNAME):
	install -d $(INSTALLDIR)/$(PROGDIR)
	install -m 0544 $(PROGNAME) $(INSTALLDIR)/$(PROGDIR)/$(PROGNAME)

