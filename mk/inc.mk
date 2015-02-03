.PHONY: all includes clean incdir incs
ALL_TARGET+= includes

ifneq ($(INCDIR)z,z)
ALL_TARGET+= incdir
endif

ifneq ($(INCS)z,z)
ALL_TARGET+= incs
endif

incdir:
	install -d ${INSTALLINCDIR}/${INCDIR}

incs:
	install -m 0644 ${INCS} ${INSTALLINCDIR}/${INCDIR}/ 


includes:
	for dir in $(INCSUBDIRS); do $(MAKE) -C $$dir; done
