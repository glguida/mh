#include <machine/asm.h>

ENTRY(__sighdlr)
	push %eax
	call _C_LABEL(sys_sighandler)
	push %eax
	call _C_LABEL(sys_xcptreturn)
END(__sighdlr)
