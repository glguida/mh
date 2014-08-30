include $(SRCROOT)/ukern.config

CFLAGS+= -D_DREX_SOURCE -I$(DISTDIR)/usr/include \
	-fno-builtin -nostdinc -nostdlib -Wall
ASFLAGS+= -D_DREX_SOURCE -I$(DISTDIR)/usr/include \
	-fno-builtin -nostdinc -nostdlib -Wall -D_ASSEMBLER

DISTINCDIR= $(DISTDIR)/usr/include
include $(MKDIR)/$(MACHINE)/rules.mk

