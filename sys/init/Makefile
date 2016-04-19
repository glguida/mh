SRCROOT=../..
include $(SRCROOT)/mk/mk.conf

SRCS+= main.c
PROGNAME= init
PROGDIR=sys
NOINST=yes

LDADD+= -lacpi -ldrex -lc -ldrex -lcrt -luk

include $(MAKEDIR)/obj.mk
include $(MAKEDIR)/sysprog.mk
include $(MAKEDIR)/def.mk
