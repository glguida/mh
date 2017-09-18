SRCROOT=$(PWD)/..
MKDIR=$(SRCROOT)/mk
MK_CONF_NO_RULES_MK_INCLUDE=y
include $(MKDIR)/mk.conf

CP=? cp
MAKEOPT?=-j4
FTP?=curl -O

TOOLDIR= $(PWD)/$(SRCROOT)/toolchain
TOOLBUILDDIR= $(TOOLDIR)/build
TOOLPREFIXDIR?= $(PWD)/install


BUILD_TARGET= i686-elf-murgia

BINUTILS= binutils-2.26
GMP= gmp-6.1.0
MPC= mpc-1.0.3
MPFR= mpfr-3.1.4
GCC= gcc-6.1.0
NEWLIB= newlib-2.5.0.20170323

GMP_CONFIGURE_OPTS=					\
			--prefix $(TOOLPREFIXDIR) \
			--with-gnu-ld;


MPFR_CONFIGURE_OPTS=					\
			--with-gnu-ld			\
			--prefix $(TOOLPREFIXDIR) 	\
			--target=$(BUILD_TARGET)	\
			--with-gmp-build=$(TOOLBUILDDIR)/gmp;


MPC_CONFIGURE_OPTS=					\
			--with-gnu-ld			\
			--prefix $(TOOLPREFIXDIR) 	\
			--target=$(BUILD_TARGET)	\
			--with-gmp=$(TOOLPREFIXDIR) 	\
			--with-mpfr=$(TOOLPREFIXDIR);


BINUTILS_CONFIGURE_OPTS=				\
			--with-gnu-ld			\
			--prefix $(TOOLPREFIXDIR) 	\
			--target=$(BUILD_TARGET)	\
			--without-isl			\
			--without-libs			\
			--without-headers		\
			--with-gmp=$(TOOLPREFIXDIR) 	\
			--with-mpfr=$(TOOLPREFIXDIR)	\
			--with-mpc=$(TOOLPREFIXDIR);


GCC_CONFIGURE_OPTS=					\
			--prefix $(TOOLPREFIXDIR) 	\
			--with-gnu-ld			\
			--target=$(BUILD_TARGET)	\
			--enable-languages=c		\
			--disable-libssp		\
			--disable-libquadmath		\
			--without-libs			\
			--without-headers		\
			--with-newlib			\
			--with-build-sysroot=$(INSTALLDIR) \
			--with-gmp=$(TOOLPREFIXDIR) 	\
			--with-mpfr=$(TOOLPREFIXDIR)	\
			--with-mpc=$(TOOLPREFIXDIR);


EXTS += $(BINUTILS) $(GMP) $(MPFR) $(MPC) $(GCC) $(NEWLIB)

.PHONY: help populate gmp mpfr mpc binutils gcc install
ALL_TARGET+= binutils gcc

help:
	@echo "Run 'make populate' to download the toolchain sources, 'make all'"
	@echo "to compile, 'make install' to install."
	@echo "Be sure to run 'make headers' in the main source directory"
	@echo "before compiling the tools."

populate:
	(cd $(EXTSDIR); \
	$(FTP) ftp.gnu.org:/pub/gnu/gmp/$(GMP).tar.bz2; \
	$(FTP) ftp.gnu.org:/pub/gnu/mpfr/$(MPFR).tar.bz2; \
	$(FTP) ftp.gnu.org:/pub/gnu/mpc/$(MPC).tar.gz; \
	$(FTP) ftp.gnu.org:/pub/gnu/binutils/$(BINUTILS).tar.bz2; \
	$(FTP) ftp.gnu.org:/pub/gnu/gcc/$(GCC)/$(GCC).tar.bz2; \
	$(FTP) sourceware.org/pub/newlib/$(NEWLIB).tar.gz)

gmp:
	mkdir -p $(TOOLBUILDDIR)/gmp;
	(cd $(TOOLBUILDDIR)/gmp; $(EXTSDIR)/$(GMP)/configure $(GMP_CONFIGURE_OPTS))
	$(MAKE) $(MAKEOPT) -C $(TOOLBUILDDIR)/gmp
	$(MAKE) $(MAKEOPT) -C $(TOOLBUILDDIR)/gmp install

mpfr:
	mkdir -p $(TOOLBUILDDIR)/mpfr;
	(cd $(TOOLBUILDDIR)/mpfr; $(EXTSDIR)/$(MPFR)/configure $(MPFR_CONFIGURE_OPTS))
	$(MAKE) $(MAKEOPT) -C $(TOOLBUILDDIR)/mpfr
	$(MAKE) $(MAKEOPT) -C $(TOOLBUILDDIR)/mpfr install

mpc:
	mkdir -p $(TOOLBUILDDIR)/mpc;
	(cd $(TOOLBUILDDIR)/mpc; $(EXTSDIR)/$(MPC)/configure $(MPC_CONFIGURE_OPTS))
	$(MAKE) $(MAKEOPT) -C $(TOOLBUILDDIR)/mpc
	$(MAKE) $(MAKEOPT) -C $(TOOLBUILDDIR)/mpc install

# Ensure we can find the just compiled libraries during gcc bootstrap
export LDFLAGS=-Wl,-rpath,$(TOOLPREFIXDIR)/lib
export CFLAGS=-Wno-error

binutils: gmp mpfr mpc
	mkdir -p $(TOOLBUILDDIR)/binutils;
	(cd $(TOOLBUILDDIR)/binutils; $(EXTSDIR)/$(BINUTILS)/configure $(BINUTILS_CONFIGURE_OPTS))
	$(MAKE) $(MAKEOPT) -C $(TOOLBUILDDIR)/binutils
	$(MAKE) $(MAKEOPT) -C $(TOOLBUILDDIR)/binutils install

gcc: binutils gmp mpfr mpc
	-rm $(EXTSDIR)/$(GCC)/newlib
	ln -s $(EXTSDIR)/$(NEWLIB)/newlib $(EXTSDIR)/$(GCC)/newlib
	mkdir -p $(TOOLBUILDDIR)/gcc;
	(cd $(TOOLBUILDDIR)/gcc; $(EXTSDIR)/$(GCC)/configure $(GCC_CONFIGURE_OPTS))
	$(MAKE) $(MAKEOPT) -C $(TOOLBUILDDIR)/gcc
	$(MAKE) $(MAKEOPT) -C $(TOOLBUILDDIR)/gcc install

include $(MKDIR)/exts.mk
include $(MKDIR)/def.mk
