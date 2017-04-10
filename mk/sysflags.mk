# Standard programs options
CPPFLAGS+= -isystem $(INSTALLDIR)/usr/include
ASFLAGS+= -isystem $(INSTALLDIR)/usr/include
LDFLAGS+= -B$(INSTALLDIR)/lib -L $(INSTALLDIR)/usr/lib

# System programs specific flags
LDFLAGS+= -L$(INSTALLDIR)/lib

LDADD+= 
