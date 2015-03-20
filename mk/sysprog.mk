.PHONY: clean_$(PROGNAME) install_$(PROGNAME)
ALL_TARGET+= install_$(PROGNAME)
CLEAN_TARGET+= clean_$(PROGNAME)
INSTALL_TARGET+= install_$(PROGNAME)

# Standard programs options
CPPFLAGS+= -isystem $(INSTALLDIR)/usr/include
CFLAGS+= -T $(SRCROOT)/mk/elf_i386.ld
ASFLAGS+= -isystem $(INSTALLDIR)/usr/include
LDFLAGS+= -B $(INSTALLDIR)/lib -B $(INSTALLDIR)/usr/lib

# System programs specific flags
LDFLAGS+= -L $(INSTALLDIR)/lib/sys
CRT0= $(INSTALLDIR)/lib/sys/crt0.o
CRTEND=
LDADD+= -lcrt

$(PROGNAME): $(OBJS)
	$(CC) -o $(PROGNAME) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) $(USRLDFLAGS) $(CRT0) $(OBJS) $(LDADD) $(CRTEND)

clean_$(PROGNAME):
	-rm $(OBJS) $(PROGNAME)

install_$(PROGNAME): $(PROGNAME)
	install -d $(INSTALLDIR)/$(PROGDIR)
	install -m 0544 $(PROGNAME) $(INSTALLDIR)/$(PROGDIR)/$(PROGNAME)

