SRCROOT=..
MKDIR=$(SRCROOT)/mk
MK_CONF_NO_RULES_MK_INCLUDE=y
include $(MKDIR)/mk.conf

FTP?=ftp

TOOLDIR= $(SRCROOT)/toolchain
TOOLPREFIXDIR= $(PWD)/$(TOOLDIR)/install
TOOLBUILDDIR= $(TOOLDIR)/build

BUILD_TARGET= i686-elf

BINUTILS= binutils-2.26
GMP= gmp-6.1.0
MPC= mpc-1.0.3
MPFR= mpfr-3.1.4

# If you change GCC version, be sure to change the path to libgcc in mk/
GCC= gcc-6.1.0

EXTS += $(BINUTILS) $(GMP) $(MPFR) $(MPC) $(GCC)

.PHONY: gmp mpfr mpc binutils gcc populate help
ALL_TARGET+= binutils gcc

help:
	@echo "Run 'make populate' to download toolchain, 'make all' to compile."
	@echo "Be sure to run 'make includes' in the main source directory"
	@echo "before compiling GCC."

populate:
	(cd $(EXTSDIR); \
	$(FTP) ftp.gnu.org:/pub/gnu/gmp/$(GMP).tar.bz2; \
	$(FTP) ftp.gnu.org:/pub/gnu/mpfr/$(MPFR).tar.bz2; \
	$(FTP) ftp.gnu.org:/pub/gnu/mpc/$(MPC).tar.gz; \
	$(FTP) ftp.gnu.org:/pub/gnu/binutils/$(BINUTILS).tar.bz2; \
	$(FTP) ftp.gnu.org:/pub/gnu/gcc/$(GCC)/$(GCC).tar.bz2;)

gmp:
	(						\
		mkdir -p $(TOOLBUILDDIR)/gmp;		\
		cd $(TOOLBUILDDIR)/gmp; 		\
		$(PWD)/$(EXTSDIR)/$(GMP)/configure 	\
			--prefix $(TOOLPREFIXDIR) 	\
			--with-gnu-ld;			\
		$(MAKE) install 			\
	)

mpfr: gmp
	(						\
		mkdir $(TOOLBUILDDIR)/mpfr;		\
		cd $(TOOLBUILDDIR)/mpfr; 		\
		$(PWD)/$(EXTSDIR)/$(MPFR)/configure 	\
			--prefix $(TOOLPREFIXDIR) 	\
			--with-gnu-ld			\
			--target=$(BUILD_TARGET)	\
			--with-gmp-build=$(PWD)/$(TOOLBUILDDIR)/gmp; \
		$(MAKE) install 			\
	)

mpc: gmp mpfr
	(						\
		mkdir $(TOOLBUILDDIR)/mpc;		\
		cd $(TOOLBUILDDIR)/mpc; 		\
		$(PWD)/$(EXTSDIR)/$(MPC)/configure 	\
			--prefix $(TOOLPREFIXDIR) 	\
			--with-gnu-ld			\
			--target=$(BUILD_TARGET)	\
			--with-gmp=$(TOOLPREFIXDIR) 	\
			--with-mpfr=$(TOOLPREFIXDIR);	\
		$(MAKE) install 			\
	)

binutils: gmp mpfr mpc
	(						\
		mkdir $(TOOLBUILDDIR)/binutils;		\
		cd $(TOOLBUILDDIR)/binutils; 		\
		$(PWD)/$(EXTSDIR)/$(BINUTILS)/configure	\
			--prefix $(TOOLPREFIXDIR) 	\
			--with-gnu-ld			\
			--target=$(BUILD_TARGET)	\
			--without-isl			\
			--with-headers=$(PWD)/$(INSTALLDIR)/usr/include \
			--without-libs			\
			--with-gmp=$(TOOLPREFIXDIR) 	\
			--with-mpfr=$(TOOLPREFIXDIR)	\
			--with-mpc=$(TOOLPREFIXDIR);	\
		$(MAKE);				\
		$(MAKE) install 			\
	)

gcc: binutils gmp mpfr mpc
	(						\
		mkdir $(TOOLBUILDDIR)/gcc; 		\
		cd $(TOOLBUILDDIR)/gcc; 		\
		$(PWD)/$(EXTSDIR)/$(GCC)/configure	\
			--prefix $(TOOLPREFIXDIR) 	\
			--with-gnu-ld			\
			--target=$(BUILD_TARGET)	\
			--enable-languages=c		\
			--disable-libssp		\
			--disable-libquadmath		\
			--with-headers=$(PWD)/$(INSTALLDIR)/usr/include \
			--without-libs			\
			--with-gmp=$(TOOLPREFIXDIR) 	\
			--with-mpfr=$(TOOLPREFIXDIR)	\
			--with-mpc=$(TOOLPREFIXDIR);	\
		$(MAKE);				\
		$(MAKE) install 			\
	)

include $(MKDIR)/exts.mk
include $(MKDIR)/def.mk
