.PHONY: clean_lib$(LIBNAME) install_lib$(LIBAME)
ALL_TARGET+= lib$(LIBNAME).a
CLEAN_TARGET+= clean_lib$(LIBNAME)
ifeq ($(NOINST)z, z)
	INSTALL_TARGET+= install_lib$(LIBNAME)
endif

lib$(LIBNAME).a: $(OBJS)
	$(AR) r $@ $(OBJS)

install_lib$(LIBNAME): lib$(LIBNAME).a
	install -d $(INSTALLDIR)/$(LIBDIR)
	install -m 0644 lib$(LIBNAME).a  $(INSTALLDIR)/$(LIBDIR)/lib$(LIBNAME).a

clean_lib$(LIBNAME):
	-rm $(OBJS) lib$(LIBNAME).a
