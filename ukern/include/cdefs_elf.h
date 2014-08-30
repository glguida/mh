#ifndef __uk_cdefs_elf_h
#define __uk_cdefs_elf_h

#include <machine/uk/asm.h>

#define __weak_alias(alias,sym)			\
  __asm(".weak " _C_LABEL_STRING(alias) "\n"	\
	_C_LABEL_STRING(alias) " = " _C_LABEL_STRING(sym))

#endif
