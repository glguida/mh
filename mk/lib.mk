.PHONY: clean_lib$(LIBNAME) install_lib$(LIBNAME)
ALL_TARGET+= install_lib$(LIBNAME)
CLEAN_TARGET+= clean_lib$(LIBNAME)

lib$(LIBNAME).a: $(OBJS)
	$(AR) r $@ $(OBJS)

install_lib$(LIBNAME): lib$(LIBNAME).a
ifeq ($(NOINST)z, z)
	install -d $(INSTALLDIR)/$(LIBDIR)
	install -m 0644 lib$(LIBNAME).a  $(INSTALLDIR)/$(LIBDIR)/lib$(LIBNAME).a
endif



clean_lib$(LIBNAME):
	-rm $(OBJS) lib$(LIBNAME).a
