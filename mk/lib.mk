.PHONY: clean_lib$(LIBNAME) install_lib$(LIBNAME)
ALL_TARGET+= install_lib$(LIBNAME)
CLEAN_TARGET+= clean_lib$(LIBNAME)

include $(MKDIR)/sysflags.mk

lib$(LIBNAME).a: $(OBJS)
ifneq ($(OBJS)z,z)
	$(AR) r $@ $(OBJS)
	$(RANLIB) $@
endif

install_lib$(LIBNAME): lib$(LIBNAME).a $(EXTOBJS)
ifeq ($(NOINST)z, z)
ifneq ($(OBJS)z,z)
	install -d $(INSTALLDIR)/$(LIBDIR)
	install -m 0644 lib$(LIBNAME).a  $(INSTALLDIR)/$(LIBDIR)/lib$(LIBNAME).a
endif
ifneq ($(EXTOBJS)z,z)
	install -m 0644 $(EXTOBJS) $(INSTALLDIR)$(LIBDIR)/
endif
endif

clean_lib$(LIBNAME):
	-rm $(OBJS) $(EXTOBJS) lib$(LIBNAME).a
