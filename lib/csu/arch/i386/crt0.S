#include <machine/asm.h>
#include <machine/asm.h>
#include <machine/uk/ukparam.h>
#include <machine/vmparam.h>	
#include <uk/sys.h>

	.data
	.comm .Linit_stack, 256

	.text
	.hidden ___start
STRONG_ALIAS(_start,__start)
_ENTRY(__start)
	/* Enable paging and switch stack */
	mov $(.Linit_stack + 256), %esp
	call siginit
	pushl $0
	pushl $(USRSTACK - 4)
	pushl $2f
	pushl $0
	mov $SYS_IRET, %eax
	int $VECT_SYSC	
2:	
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	call	___start
	mov $SYS_DIE, %eax
	int $VECT_SYSC
