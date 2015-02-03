#include <machine/asm.h>
#include <machine/vmparam.h>

.data
.comm stack, PAGE_SIZE

.text
.code32

STRONG_ALIAS(_start,__start)
_ENTRY(__start)
	movl $(stack + PAGE_SIZE), %esp	
	call	_C_LABEL(___start)

___endloop:
	jmp ___endloop;


