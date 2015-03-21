#ifndef _uk_sys_h_
#define _uk_sys_h_


#define SYS_DIE 0
#define SYS_XCPTENTRY 1
#define SYS_XCPTRET 2

/* System-processes only */
#define SYS_PUTC 0x1000

#define XCPT_PGFAULT 0

#ifdef _UKERNEL
int sys_putc(int ch);
int sys_xcptentry(vaddr_t, vaddr_t);
int sys_die(void);
#endif

#endif
