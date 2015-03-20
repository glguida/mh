
#define __systrap		\
	"movl %0, %%eax;"	\
	"int $0x80;"

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
