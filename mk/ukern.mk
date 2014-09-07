CFLAGS += -D_UKERNEL -I .
ASFLAGS += -D_UKERNEL
LDFLAGS += -T $(MKDIR)/ukern.ld

OBJDIR=$(SRCROOT)/ukern/.objs

include $(MKDIR)/rules.mk

