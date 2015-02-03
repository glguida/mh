CFLAGS += -D_UKERNEL -I .
ASFLAGS += -D_UKERNEL
LDFLAGS += -T $(MAKEDIR)/ukern.ld

include $(MAKEDIR)/rules.mk

