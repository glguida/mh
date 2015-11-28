# Rules:
#
# - If an archdir exits, its Makefile.inc will be included.
# - archdirs will have precedence over generic ones.


COMMON_DIR=$(SRCROOT)/common/libc

ifneq ($(wildcard  $(COMMON_DIR)/$(MACHINE)/string),)
	SUBDIRS_MKINC+= $(COMMON_DIR)/$(MACHINE)/string
endif
SUBDIRS_MKINC+= $(COMMON_DIR)/string
