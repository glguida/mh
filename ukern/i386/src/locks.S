#include <machine/uk/asm.h>
	
ENTRY(spinlock)
1:	movl    $1, %ecx
	cmpxchg %ecx, (%eax)
	je     2f
	ret
2:	pause
	cmpl 	$0, (%eax)
	je 	1b
	jmp 	2b
END(spinlock)
	

ENTRY(spinunlock)
	movl    $0, (%eax)
	ret
END(spinunlock)
