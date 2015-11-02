include $(MAKEDIR)/rules.mk

CFLAGS += -D_UKERNEL -I . -I $(INSTALLDIR)/usr/include/microkernel
ASFLAGS += -D_UKERNEL -I . -I $(INSTALLDIR)/usr/include/microkernel
LDFLAGS += -T $(MAKEDIR)/ukern.ld

