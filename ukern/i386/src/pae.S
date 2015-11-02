#include <machine/uk/asm.h>
#include <uk/param.h>

ENTRY(__getpdptr)
	mov %cr3, %eax
	ret
END(__getpdptr)

ENTRY(__setpdptr)
	sub $UKERNBASE, %eax
	mov %eax, %cr3
	ret
END(__setpdptr)
