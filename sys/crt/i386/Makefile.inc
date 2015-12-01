VPATH+= i386
SRCS+=
ALL_TARGET+= install_crt0.o
CLEAN_TARGET+= clean_crt0.o

VPATH+= $(SRCROOT)/exts/compiler_rt/lib/builtins/i386
SRCS+= udivdi3.S umoddi3

.PHONY: install_crt0.o clean_crt0.o

install_crt0.o: crt0.o
	install -d $(INSTALLDIR)/lib/sys
	install -m 0500 crt0.o $(INSTALLDIR)/lib/sys

clean_crt0.o:
	-rm crt0.o

