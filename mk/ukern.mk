include $(MAKEDIR)/rules.mk

ifneq ($(SERIAL_OUTPUT)z, z)
CFLAGS+=-DSERIAL_PUTC
endif

CFLAGS += -D_UKERNEL -I . -I $(INSTALLDIR)/usr/include/microkernel
ASFLAGS += -D_UKERNEL -I . -I $(INSTALLDIR)/usr/include/microkernel
LDFLAGS += -T $(MAKEDIR)/ukern.ld

