/* *INDENT-OFF* */ /* Imported from NetBSD -- MHDIFFIGNORE */
/* $NetBSD: crt0.S,v 1.3 2011/07/01 02:59:05 joerg Exp $ */

/*
 * Copyright (c) 1998 Christos Zoulas
 * Copyright (c) 1995 Christopher G. Demetriou
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.NetBSD.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * <<Id: LICENSE,v 1.2 2000/06/14 15:57:33 cgd Exp>>
 */

#include <machine/asm.h>

#ifdef _DREX_SOURCE
	.data
	.comm .Linit_stack, 256
	.text
#endif

RCSID("$NetBSD: crt0.S,v 1.3 2011/07/01 02:59:05 joerg Exp $")

	.hidden ___start

#ifdef _DREX_SOURCE
#include <machine/asm.h>
#include <machine/uk/ukparam.h>
#include <machine/vmparam.h>	
#include <uk/sys.h>
#endif	
	
	
STRONG_ALIAS(_start,__start)
_ENTRY(__start)
#ifdef _DREX_SOURCE
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
#endif	
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	call	___start
#ifdef _DREX_SOURCE	
	mov $SYS_DIE, %eax
	int $VECT_SYSC
#endif
