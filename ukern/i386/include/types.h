#ifndef __i386_types_h
#define __i386_types_h

typedef u_long lock_t;

typedef unsigned long long paddr_t;
typedef u_long vaddr_t;
typedef unsigned pfn_t;

#if defined(_UKERNEL)

typedef unsigned long jmp_buf[6];

struct usrentry {
	char data[17 * 4];
};

#endif

#endif
