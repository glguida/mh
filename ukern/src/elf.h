#ifndef _uk_elf_h
#define _uk_elf_h

struct elfph {
	uint32_t type;
	uint32_t off;
	uint32_t va;
	uint32_t pa;
	uint32_t fsize;
	uint32_t msize;
	uint32_t flags;
	uint32_t align;
};

struct elfsh {
	uint32_t name;
	uint32_t type;
	uint32_t flags;
	uint32_t addr;
	uint32_t off;
	uint32_t size;
	uint32_t lnk;
	uint32_t info;
	uint32_t align;
	uint32_t shent_size;
};

struct elfhdr {
	uint8_t id[16];
	uint16_t type;
	uint16_t mach;
	uint32_t ver;
	uint32_t entry;
	uint32_t phoff;
	uint32_t shoff;
	uint32_t flags;
	uint16_t eh_size;
	uint16_t phent_size;
	uint16_t phs;
	uint16_t shent_size;
	uint16_t shs;
	uint16_t shstrndx;
} __packed;
#define ET_EXEC		2
#define EM_386		3
#define EV_CURRENT	1

#define SHT_PROGBITS 	1
#define SHT_NOBITS	8

#define PHT_NULL	0
#define PHT_LOAD	1
#define PHT_DYNAMIC	2
#define PHT_INTERP	3
#define PHT_NOTE	4
#define PHT_SHLIB	5
#define PHT_PHDR	6
#define PHT_TLS		7

#define PHF_X		1
#define PHF_W		2
#define PFH_R		4

#endif
