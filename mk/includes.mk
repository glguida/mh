include $(MKDIR)/rules.mk

.PHONY: includes

includes:
	for dir in $(INCSUBDIRS); do $(MAKE) -C $$dir includes; done
	install -d ${DISTINCDIR}/${INCINSTDIR}
	install -m 0644 ${INCS} ${DISTINCDIR}/${INCINSTDIR}/

