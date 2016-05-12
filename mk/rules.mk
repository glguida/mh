ifeq ($(MACHINE),i386)
TOOLCHAINTARGET=i686-elf-murgia
endif

TOOLCHAINBIN= $(SRCROOT)/toolchain/install/bin
export CC= $(TOOLCHAINBIN)/$(TOOLCHAINTARGET)-gcc
export AS= $(TOOLCHAINBIN)/$(TOOLCHAINTARGET)-as
export LD= $(TOOLCHAINBIN)/$(TOOLCHAINTARGET)-ld
export AR= $(TOOLCHAINBIN)/$(TOOLCHAINTARGET)-ar
export OBJCOPY= $(TOOLCHAINBIN)/$(TOOLCHAINTARGET)-objcopy
export RANLIB= $(TOOLCHAINBIN)/$(TOOLCHAINTARGET)-ranlib

LN= ln
INSTALL= install

INSTALLINCDIR= $(INSTALLDIR)/usr/include
INSTALLUKINCDIR= $(INSTALLDIR)/usr/include/microkernel

CFLAGS+= -D_DREX_SOURCE -I$(INSTALLDIR)/usr/include \
	-D_DREX_MACHINE=$(MACHINE)
ASFLAGS+= -D_DREX_SOURCE -I$(INSTALLDIR)/usr/include -D_ASSEMBLER \
	-D_DREX_MACHINE=$(MACHINE)

CFLAGS+= -fno-strict-aliasing -fno-delete-null-pointer-checks

ifneq ($(DEBUG)z,z)
CFLAGS+= -D__DEBUG -O0 -g
else
CFLAGS+= -O2 -g
endif

do_all: all

ifneq ($(SUBDIRS_MKINC)z, z)
include $(addsuffix /Makefile.inc, $(SUBDIRS_MKINC))
endif

include $(MAKEDIR)/$(MACHINE)/rules.mk

