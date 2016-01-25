# Rules:
#
# - If an archdir exits, its Makefile.inc will be included.
# - archdirs will have precedence over generic ones.


COMMON_DIR=$(SRCROOT)/common/libc

ifneq ($(wildcard  $(COMMON_DIR)/$(MACHINE)/gen),)
	SUBDIRS_MKINC+= $(COMMON_DIR)/$(MACHINE)/gen
endif
SUBDIRS_MKINC+= $(COMMON_DIR)/gen

ifneq ($(wildcard  $(COMMON_DIR)/$(MACHINE)/string),)
	SUBDIRS_MKINC+= $(COMMON_DIR)/$(MACHINE)/string
endif
SUBDIRS_MKINC+= $(COMMON_DIR)/string

ifneq ($(wildcard $(COMMON_DIR)/$(MACHINE)/atomic),)
        SUBDIRS_MKINC+= $(COMMON_DIR)/$(MACHINE)/atomic
endif
