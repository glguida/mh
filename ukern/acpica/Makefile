SRCROOT=../..
include $(SRCROOT)/mk/mk.conf
OBJDIR=$(SRCROOT)/ukern/acpica/.objdir
ALL_PREDEP = exts
EXTS += acpica

ifneq ($(DEBUG)z,z)
CFLAGS+= -DACPICA_DEBUG
endif

include $(MAKEDIR)/ukern.mk

LIBNAME=kacpica
NOINST=y

SRCS+= acpi.c acpios.c
VPATH+= $(SRCROOT)/acpica

CFLAGS+= -I $(EXTSDIR)/acpica/include
SRCS+= tbxfroot.c tbxface.c tbxfload.c \
	tbprint.c tbinstal.c tbfind.c \
	tbfadt.c tbutils.c
VPATH+= $(EXTSDIR)/acpica/tables/
SRCS+= utclib.c utxferror.c utexcep.c \
	utresrc.c utglobal.c utalloc.c \
	utmisc.c utstate.c utmutex.c \
	utownerid.c utstring.c utlock.c \
	utmath.c utobject.c utdecode.c \
	utdelete.c utaddress.c utosi.c \
	uterror.c utcopy.c uteval.c \
	utids.c utbuffer.c utpredef.c \
	utxfinit.c utxface.c utinit.c
VPATH+= $(EXTSDIR)/acpica/utilities/
SRCS+= nsalloc.c nswalk.c nsobject.c \
	nsnames.c nsutils.c nsaccess.c \
	nssearch.c nseval.c nsarguments.c \
	nsload.c nsxfname.c nsxfobj.c \
	nspredef.c nsprepkg.c nsxfeval.c \
	nsrepair.c nsrepair2.c nsparse.c \
	nsconvert.c  nsinit.c
VPATH+= $(EXTSDIR)/acpica/namespace/
SRCS+= evgpeblk.c evxfgpe.c evgpeutil.c \
	evgpeinit.c evgpe.c evsci.c \
	evevent.c evglock.c evxface.c \
	evxfevnt.c evmisc.c evregion.c \
	evhandler.c evrgnini.c evxfregn.c
VPATH+= $(EXTSDIR)/acpica/events/
SRCS+= hwgpe.c hwregs.c hwxface.c \
	hwvalid.c hwacpi.c hwpci.c
VPATH+= $(EXTSDIR)/acpica/hardware/
SRCS+= dswscope.c dswstate.c dsmthdat.c \
	dswload.c dsfield.c dsmethod.c \
	dsutils.c dswexec.c dsopcode.c \
	dsobject.c dsargs.c dsinit.c \
	dscontrol.c dswload2.c
VPATH+= $(EXTSDIR)/acpica/dispatcher/
SRCS+= exmutex.c exutils.c exfield.c \
	exoparg1.c exoparg2.c exoparg3.c \
	exoparg6.c exconfig.c exmisc.c \
	exconvrt.c exstore.c exresop.c \
	exresnte.c exresolv.c exsystem.c \
	exprep.c exregion.c exnames.c \
	exfldio.c exdebug.c excreate.c \
	exstoren.c exstorob.c
VPATH+= $(EXTSDIR)/acpica/executer/
SRCS+= psxface.c pstree.c psparse.c \
	psscope.c psargs.c psobject.c \
	psopinfo.c psutils.c pswalk.c \
	psopcode.c psloop.c
VPATH+= $(EXTSDIR)/acpica/parser/

UKINCDIR= acpica
UKINCS= acpica.h

include $(MAKEDIR)/ukinc.mk
include $(MAKEDIR)/obj.mk
include $(MAKEDIR)/exts.mk
include $(MAKEDIR)/lib.mk
include $(MAKEDIR)/def.mk

