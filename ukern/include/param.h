#ifndef __uk_param_h
#define __uk_param_h

#ifndef _ASSEMBLER
#include <uk/types.h>
#endif

#ifdef _UKERNEL
#include <machine/uk/param.h>
#endif

#define __usraddr(_a) (((_a) >= USERBASE) && ((_a) <= USEREND))

#endif
