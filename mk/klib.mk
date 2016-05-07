.PHONY: clean_lib$(LIBNAME)
ALL_TARGET+= lib$(LIBNAME).a
CLEAN_TARGET+= clean_lib$(LIBNAME)

lib$(LIBNAME).a: $(OBJS)
ifneq ($(OBJS)z,z)
	$(AR) r $@ $(OBJS)
	$(RANLIB) $@
endif

clean_lib$(LIBNAME):
	-rm $(OBJS) lib$(LIBNAME).a
