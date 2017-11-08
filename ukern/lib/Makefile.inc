# Rules:
#
# - If an archdir exits, its Makefile.inc will be included.
# - archdirs will have precedence over generic ones.

ifneq ($(wildcard  lib/arch/$(MACHINE)/gen),)
      SUBDIRS_MKINC+= lib/arch/$(MACHINE)/gen
endif
SUBDIRS_MKINC+= lib/gen

ifneq ($(wildcard  lib/arch/$(MACHINE)/string),)
      SUBDIRS_MKINC+= lib/arch/$(MACHINE)/string
endif
SUBDIRS_MKINC+= lib/string

SUBDIRS_MKINC+= lib/logio