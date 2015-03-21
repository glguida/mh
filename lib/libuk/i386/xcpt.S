#include <machine/uk/asm.h>

ENTRY(__sighdlr)
	push %esi
	push %edi
	push %eax
	call _C_LABEL(sys_sighandler)
	push %eax
	call _C_LABEL(sys_xcptreturn)
END(__sighdlr)
