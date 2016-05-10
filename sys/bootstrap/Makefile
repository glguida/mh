SRCROOT=../../
include $(SRCROOT)/mk/mk.conf

SRCS+= bootstrap.c
PROGNAME= bootstrap
PROGDIR=sys
NOINST=yes

LDADD+= -lacpi -ldrex

include $(MAKEDIR)/obj.mk
include $(MAKEDIR)/sysprog.mk
include $(MAKEDIR)/def.mk
