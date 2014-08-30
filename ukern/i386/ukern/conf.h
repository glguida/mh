#ifndef __conf_h
#define __conf_h

#define KCS 0x08
#define KDS 0x10
#define RCS 0x18
#define RDS 0x20

#define SEG16_ADDR(_a) (((_a) >> 4) & 0xf000)
#define OFF16_ADDR(_a) ((_a) & 0xffff)

#define ADDR_CODE16 0x10000
#define ADDR_STCK16 0x2fffe
#define ADDR_SMAP16 0x30000

#endif
