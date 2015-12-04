/*
 * Copyright (c) 2015, Gianluca Guida
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <machine/uk/ukparam.h>

#define _str(_x) #_x
#define ___systrap(_vect)	\
	"movl %0, %%eax;"	\
	"int $" _str(_vect)

#define __systrap ___systrap(VECT_SYSC)


#define __syscall0(__sys, __ret)	\
	asm volatile(__systrap		\
		: "=a" (__ret)		\
		: "a" (__sys));

#define __syscall1(__sys, a1, __ret)	\
	asm volatile(__systrap		\
		: "=a" (__ret)		\
		: "a" (__sys),		\
		  "D" (a1));

#define __syscall2(__sys, a1, a2, __ret)\
	asm volatile(__systrap		\
		: "=a" (__ret)		\
		: "a" (__sys),		\
		  "D" (a1),		\
		  "S" (a2));


#define __syscall3(__sys, a1, a2, a3, __ret)	\
	asm volatile(__systrap			\
		: "=a" (__ret)			\
		: "a" (__sys),			\
		  "D" (a1),			\
		  "S" (a2), 			\
		  "c" (a3));


#define __syscall4(__sys, a1, a2, a3, a4, __ret)\
	asm volatile(__systrap			\
		: "=a" (__ret)			\
		: "a" (__sys),			\
		  "D" (a1),			\
		  "S" (a2), 			\
		  "c" (a3),			\
		  "d" (a4));

#define __syscall5(__sys, a1, a2, a3, a4, a5, __ret)	\
	asm volatile(__systrap				\
		: "=a" (__ret)				\
		: "a" (__sys),				\
		  "D" (a1),				\
		  "S" (a2), 				\
		  "c" (a3),				\
		  "d" (a4),				\
		  "b" (a5));
