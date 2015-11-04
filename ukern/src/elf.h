/*
 * Copyright (c) 2015, Gianluca Guida
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


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
