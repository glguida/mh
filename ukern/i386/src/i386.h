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


#ifndef _i386_i386_h
#define _i386_i386_h

#include <uk/types.h>

/* Processor specific */

struct tss {
	uint16_t ptl, tmp0;
	uint32_t esp0;
	uint16_t ss0, tmp1;
	uint32_t esp1;
	uint16_t ss1, tmp2;
	uint32_t esp2;
	uint16_t ss2, tmp3;
	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;
	uint16_t es, tmp4;
	uint16_t cs, tmp5;
	uint16_t ss, tmp6;
	uint16_t ds, tmp7;
	uint16_t fs, tmp8;
	uint16_t gs, tmp9;
	uint16_t ldt, tmpA;
	uint16_t t_flag, iomap;
} __packed;

/* Platform */

#define SERIAL_PORT 0x3f8

static inline int inb(int port)
{
	int ret;

	asm volatile ("inb %%dx, %%al":"=a" (ret):"d"(port));
	return ret;
}

static inline void outb(int port, int val)
{
	asm volatile ("outb %%al, %%dx"::"d" (port), "a"(val));
}

static inline void _delay(void)
{
	int i;

	for (i = 0; i < 10000; i++)
		asm volatile ("pause");
}

static inline void cmos_write(uint8_t addr, uint8_t val)
{
	asm volatile ("outb %0, $0x70\n\t"
		      "outb %0, $0x71\n\t"::"r" (addr), "r"(val));
}

static inline void pic_off(void)
{
#define pic_write(_p1, _p2) do {		\
    asm volatile ("outb %0, $0x21\n\t" :: "r"((uint8_t)_p1));	\
    asm volatile ("outb %0, $0xa1\n\t" :: "r"((uint8_t)_p2));	\
  } while (0)

	pic_write(0xff, 0xff);
}

struct usrframe {
	/* segments */
	uint16_t ds;
	uint16_t es;
	uint16_t fs;
	uint16_t gs;
	/* CRs    */
	uint32_t cr2;
	uint32_t cr3;
	/* pushal */
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t espx;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	/* exception stack */
	uint32_t err;
	uint32_t eip;
	uint32_t cs;
	uint32_t eflags;
	uint32_t esp;
	uint32_t ss;
} __packed;

#define USRFRAME_SIZE (18 * 4)

int xcpt_entry(uint32_t vect, struct usrframe *f);
int intr_entry(uint32_t vect, struct usrframe *f);

#endif
