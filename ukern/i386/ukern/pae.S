#include <machine/uk/asm.h>
#include <uk/param.h>

ENTRY(__setpdptr)
	sub $UKERNBASE, %eax
	mov %eax, %cr3
	ret
END(__setpdptr)
