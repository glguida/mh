SUBDIRS+= i386/src
SUBDIRS_MKINC+= i386/src/ i386/lib/

CFLAGS+= -I $(EXTSDIR)/acpica/include
