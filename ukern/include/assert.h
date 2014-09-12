#ifndef __uk_assert_h
#define __uk_assert_h

#define assert(_e) do { if (!(_e)) panic("Assert failed: " #_e); } while(0)

#endif
