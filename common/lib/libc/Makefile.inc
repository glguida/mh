# Rules:
#
# - If an archdir exits, its Makefile.inc will be included.
# - archdirs will have precedence over generic ones.


COMMON_DIR=$(SRCROOT)/common/lib/libc

ifneq ($(wildcard  $(COMMON_DIR)/arch/$(MACHINE)/gen),)
      SUBDIRS_MKINC+= $(COMMON_DIR)/arch/$(MACHINE)/gen
endif
SUBDIRS_MKINC+= $(COMMON_DIR)/gen

ifneq ($(wildcard  $(COMMON_DIR)/arch/$(MACHINE)/string),)
      SUBDIRS_MKINC+= $(COMMON_DIR)/arch/$(MACHINE)/string
endif
SUBDIRS_MKINC+= $(COMMON_DIR)/string

ifneq ($(wildcard $(COMMON_DIR)/arch/$(MACHINE)/atomic),)
      SUBDIRS_MKINC+= $(COMMON_DIR)/arch/$(MACHINE)/atomic
endif
