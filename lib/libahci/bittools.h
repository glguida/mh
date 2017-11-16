#ifndef __bithelp_h
#define __bithelp_h


#define bitfld_set(_v, _n, _f) (((_v) & ~_n##_MASK)|(((_f) << _n##_SHIFT) & _n##_MASK))
#define bitfld_get(_v, _n) (((_v) & _n##_MASK) >> _n##_SHIFT)

#define _B(_x) (1LL < (_x))

#endif
