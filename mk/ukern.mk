include $(MAKEDIR)/rules.mk

ifneq ($(SERIAL_OUTPUT)z, z)
CFLAGS+=-DSERIAL_PUTC
endif

CFLAGS += -D_UKERNEL -I . -I $(INSTALLDIR)/usr/include/microkernel 	\
	-fno-builtin -nostdinc -nostdlib
ASFLAGS += -D_UKERNEL -I . -I $(INSTALLDIR)/usr/include/microkernel
LDFLAGS += -T$(MAKEDIR)/ukern.ld
LDADD+= `$(CC) -print-libgcc-file-name`

