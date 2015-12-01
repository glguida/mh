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


#ifndef _uk_sys_h_
#define _uk_sys_h_

/*
 * Syscalls
 */
#define SYS_DIE 0
#define SYS_INTHDLR 1
#define SYS_IRET 2
#define SYS_STI 3
#define SYS_CLI 4
#define SYS_WAIT 5
#define SYS_FORK 6

#ifndef _ASSEMBLER
typedef enum {
	MAP_NEW = 0x100,
	MAP_NONE = 0,
	MAP_RDONLY = 1,
	MAP_RDEXEC = 2,
	MAP_WRITE = 3,
	MAP_WREXEC = 4,
} sys_map_flags_t;
#endif
#define SYS_MAP  0x10
#define SYS_MOVE 0x11

#define SYS_OPEN   0x20
#define SYS_INTMAP 0x21
#define SYS_IO     0x22
#define SYS_CREAT  0x30

#ifndef _ASSEMBLER
struct sys_poll_ior {
	uint64_t port;
	uint64_t val;
};
#endif
#define SYS_POLL   0x31
#define SYS_EIO    0x32

/* System-processes only */
#define SYS_PUTC 0x1000

/*
 * Exceptions
 */
#define XCPT_PGFAULT 0
#define PG_ERR_REASON_NOTP  0
#define PG_ERR_REASON_PROT  1
#define PG_ERR_REASON_MASK  1
#define PG_ERR_INFO_COW   128
#define PG_ERR_INFO_WRITE 256

#define INTR_EXT     64


#ifdef _UKERNEL
int sys_call(int sc, unsigned long a1, unsigned long a2, unsigned long a3);
#endif

#endif
