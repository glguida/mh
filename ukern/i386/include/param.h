#ifndef __i386_param_h
#define __i386_param_h

#ifdef _UKERNEL

#define MAXCPUS         32

#define PGSHIFT         12
#define NBPG            (1 << PGSHIFT)
#define PGOFSET         (NBPG-1)
#define NPTEPG          (NBPG/(sizeof (pt_entry_t)))

#define KERNBASE        0xc0000000

#define KERNCMPOFF      0x100000 /* 1Mb */
#define KERNTEXTOFF     (KERNBASE + KERNCMPOFF)

#endif /* _UKERNEL */

#endif
