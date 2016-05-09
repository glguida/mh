# Standard programs options
CPPFLAGS+= -isystem $(INSTALLDIR)/usr/include
ASFLAGS+= -isystem $(INSTALLDIR)/usr/include
LDFLAGS+= -B$(INSTALLDIR)/lib -L /usr/lib -T $(SRCROOT)/mk/elf_i386.ld

# We don't yet support puts and putchar in libc, remove default
# library. TO BE REMOVED.
CFLAGS+= -fno-builtin

# System programs specific flags
LDFLAGS+= -L$(INSTALLDIR)/lib

LDADD+= 
