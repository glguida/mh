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


#ifndef _i386_platform_h
#define _i386_platform_h

#include <machine/uk/apic.h>

void platform_init(void);

static inline int platform_inb(unsigned port)
{
	int ret;

	asm volatile ("xor %%eax, %%eax; inb %%dx, %%al":"=a" (ret):"d"(port));
	return ret;
}

static inline int platform_inw(unsigned port)
{
	int ret;

	asm volatile ("xor %%eax, %%eax; inw %%dx, %%ax":"=a" (ret):"d"(port));
	return ret;
}

static inline int platform_inl(unsigned port)
{
	int ret;

	asm volatile ("inl %%dx, %%eax":"=a" (ret):"d"(port));
	return ret;
}

static inline void platform_outb(unsigned port, int val)
{
	asm volatile ("outb %%al, %%dx"::"d" (port), "a"(val));
}

static inline void platform_outw(unsigned port, int val)
{
	asm volatile ("outw %%ax, %%dx"::"d" (port), "a"(val));
}

static inline void platform_outl(unsigned port, int val)
{
	asm volatile ("outl %%eax, %%dx"::"d" (port), "a"(val));
}

static inline void platform_wait(void)
{

	asm volatile ("sti\n\thlt");
}

static inline void platform_irqon(unsigned irq)
{
	/* 1:1 Mapping GSI-IRQ, only ones supported for now. */
	gsi_enable(irq);
}

static inline void platform_irqoff(unsigned irq)
{
	gsi_disable(irq);
}

#endif
