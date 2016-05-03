SRCROOT=../..
MKDIR=$(SRCROOT)/mk
include $(MKDIR)/mk.conf

LIBNAME=acpi
LIBDIR=/lib

ALL_PREDEP = exts
EXTS += acpica

SRCS+=  acpios.c

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
	utxfinit.c utxface.c utinit.c \
	utdebug.c
VPATH+= $(EXTSDIR)/acpica/utilities/
SRCS+= nsalloc.c nswalk.c nsobject.c \
	nsnames.c nsutils.c nsaccess.c \
	nssearch.c nseval.c nsarguments.c \
	nsload.c nsxfname.c nsxfobj.c \
	nspredef.c nsprepkg.c nsxfeval.c \
	nsrepair.c nsrepair2.c nsparse.c \
	nsconvert.c  nsinit.c nsdump.c
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
	exstoren.c exstorob.c exdump.c
VPATH+= $(EXTSDIR)/acpica/executer/
SRCS+= psxface.c pstree.c psparse.c \
	psscope.c psargs.c psobject.c \
	psopinfo.c psutils.c pswalk.c \
	psopcode.c psloop.c
VPATH+= $(EXTSDIR)/acpica/parser/
SRCS+= rsaddr.c rscalc.c rscreate.c rsdump.c rsdumpinfo.c rsinfo.c	\
	rsio.c rsirq.c rslist.c rsmemory.c rsmisc.c rsserial.c		\
	rsutils.c rsxface.c
VPATH+= $(EXTSDIR)/acpica/resources/

ACPI_INCDIR= $(EXTSDIR)/acpica/include/
ACPI_INCS+= acapps.h acbuffer.h accommon.h acconfig.h acdebug.h		\
acdisasm.h acdispat.h acevents.h acexcep.h acglobal.h achware.h		\
acinterp.h aclocal.h acmacros.h acnames.h acnamesp.h acobject.h		\
acopcode.h acoutput.h acparser.h acpi.h acpiosxf.h acpixf.h		\
acpredef.h acresrc.h acrestyp.h acstruct.h actables.h actbl1.h		\
actbl2.h actbl3.h actbl.h actypes.h acutils.h amlcode.h amlresrc.h
ACPI_INCS+= platform/acenv.h platform/acdrex.h

INCS= $(addprefix $(ACPI_INCDIR), $(ACPI_INCS))
INCDIR= acpi

#CFLAGS+= -I $(MACHINE)
#ASFLAGS+= -I $(MACHINE)

ALL_TARGET+= makelinks
makelinks: incs
	$(INSTALL) -d ${INSTALLINCDIR}/${INCDIR}/platform
	-$(RM) ${INSTALLINCDIR}/${INCDIR}/platform/acdrex.h
	$(LN) -s ../acdrex.h ${INSTALLINCDIR}/${INCDIR}/platform/acdrex.h
	-$(RM) ${INSTALLINCDIR}/${INCDIR}/platform/acenv.h
	$(LN) -s ../acenv.h ${INSTALLINCDIR}/${INCDIR}/platform/acenv.h

include $(MKDIR)/exts.mk
include $(MKDIR)/inc.mk
include $(MKDIR)/obj.mk
include $(MKDIR)/lib.mk
include $(MKDIR)/def.mk
