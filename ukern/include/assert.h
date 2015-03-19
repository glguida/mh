#ifndef __uk_assert_h
#define __uk_assert_h

#include <lib/lib.h>

#define assert(_e) do { if (!(_e)) panic("Assert failed: " #_e); } while(0)

#ifdef __DEBUG
#define dbgassert(_e) do { if (!(_e)) panic("Assert failed: " #_e); } while(0)
#else
#define dbgassert(_e)
#endif

#endif
