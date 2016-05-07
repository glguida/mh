.PHONY: clean_$(PROGNAME) install_$(PROGNAME)
ALL_TARGET+= install_$(PROGNAME)
CLEAN_TARGET+= clean_$(PROGNAME)
INSTALL_TARGET+= install_$(PROGNAME)

include $(MAKEDIR)/sysflags.mk

$(PROGNAME): $(OBJS)
	$(CC) -o $(PROGNAME) $(LDFLAGS) $(OBJS) $(LDADD)

clean_$(PROGNAME):
	-rm $(OBJS) $(PROGNAME)

install_$(PROGNAME): $(PROGNAME)
	install -d $(INSTALLDIR)/$(PROGDIR)
	install -m 0544 $(PROGNAME) $(INSTALLDIR)/$(PROGDIR)/$(PROGNAME)

