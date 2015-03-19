#ifndef __conf_h
#define __conf_h


/* Temporary address for boot-time allocation. */
#define ADDR_CODE16   0x10000	/* 16bit code  */
#define ADDR_STCK16   0x2fffe	/* 16bit stack */
#define ADDR_SMAP16   0x30000	/* E820 map */

#define ADDR_BOOTL3   0x50000	/* L3 table for boot (32 bytes) */
#define ADDR_BOOTL2   0x51000	/* L2 table for boot (4k bytes) */
#define ADDR_BOOTGDTR 0x52000	/* boot GDTR  */
#define ADDR_BOOTGDT  0x52030	/* boot GDT */

#endif
