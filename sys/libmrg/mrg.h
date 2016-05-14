/*
 * Copyright (c) 2015-2016, Gianluca Guida
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


#ifndef __mrg_runtime_h
#define __mrg_runtime_h

#include <sys/types.h>
#include <inttypes.h>


/*
 * VM Memory Management
 */

#define VFNT_INVALID 0
#define VFNT_FREE    1
#define VFNT_RODATA  2
#define VFNT_RWDATA  3
#define VFNT_EXEC    4
#define VFNT_WREXEC  5
#define VFNT_IMPORT  6
#define VFNT_MMIO    7

vaddr_t vmap_alloc(size_t size, uint8_t type);
void vmap_free(vaddr_t va, size_t size);
void vmap_info(vaddr_t va, vaddr_t * start, size_t * size, uint8_t * type);

int brk(void *);
void *sbrk(int);


/*
 * Interrupt allocation and handling.
 */

unsigned intalloc(void);
void intfree(unsigned);
void inthandler(unsigned, void (*)(int, void *), void *);
#include "mrg_preempt.h"


/*
 * Lightweight Threads.
 */

#include "mrg_lwt.h"


/*
 * Event handling.
 */

int evtalloc(void);
void evtwait(int evt);
void __evtset(int evt);


/*
 * Device handling.
 */

struct _DEVICE;
typedef struct _DEVICE DEVICE;

DEVICE *dopen(char *devname, devmode_t mode);
int dio(DEVICE *d, uint8_t op, uint64_t *val, int evtid);
int diow(DEVICE *d, uint8_t op, uint64_t *val, int evtid);
void dclose(DEVICE *d);

#endif
