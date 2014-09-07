.PHONY: all includes clean
ALL_TARGET+= includes

includes:
	for dir in $(INCSUBDIRS); do $(MAKE) -C $$dir includes; done
	install -d ${INSTALLINCDIR}/${INCDIR}
	install -m 0644 ${INCS} ${INSTALLINCDIR}/${INCDIR}/ 
