CFLAGS += -D_UKERNEL -I .
ASFLAGS += -D_UKERNEL
LDFLAGS += -T $(MKDIR)/ukern.ld

include $(MKDIR)/rules.mk

