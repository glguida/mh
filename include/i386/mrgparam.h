#ifndef _I386_DREXPARAM_H_
#define _I386_DREXPARAM_H_

#include <machine/vmparam.h>

#define VM_MAXBRK_ADDRESS (768L << L1_SHIFT)
#define VM_MINMMAP_ADDRESS (768L << L1_SHIFT)
#define VM_MAXMMAP_ADDRESS (USRSTACK - MAXSSIZ)

#endif
