UKERN=y
SRCROOT= ..
MKDIR=$(SRCROOT)/mk
include $(MKDIR)/rules.mk

CFLAGS += -D_UKERNEL -I .
ASFLAGS += -D_UKERNEL

INCSRCDIRS+= include
SUBDIRS= $(MACHINE)/ lib/ ukern/

all: $(OBJDIR) ukern

include $(MKDIR)/subdir.mk
include $(MKDIR)/obj.mk
LDFLAGS += -T $(MKDIR)/ukern.ld

$(OBJDIR):
	mkdir -p $(OBJDIR)

.phony: install_headers

install_headers:
	for dir in $(INCSRCDIRS); do $(MAKE) -C $$dir includes; done

ukern: install_headers $(OBJS)
	$(LD) $(LDFLAGS) -o $(DISTDIR)/$@ $(addprefix $(OBJDIR)/, $(OBJS))


