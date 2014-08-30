#ifndef __i386_locks_h
#define __i386_locks_h

#include <uk/cdefs.h>
#include <uk/types.h>

#ifdef _UKERNEL

__regparm void spinlock(lock_t *l);
__regparm void spinunlock(lock_t *l);

#endif /* _UKERNEL */

#endif
