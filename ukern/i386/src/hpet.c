/*
 * Copyright (c) 2017, Gianluca Guida
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

#include <uk/types.h>
#include <uk/vmap.h>
#include <lib/lib.h>

/* ACPICA includes */
#include "acpi.h"

#define HPET_SIZE 0x1000

volatile void *hpet_base;
uint32_t period_femto;

static uint64_t hpet_read(uint16_t offset)
{
	uint32_t hi1, hi2, lo;
	uint64_t ret;
	volatile uint32_t *ptr = hpet_base + offset;

	do {
		hi1 = *(ptr + 8);
		lo = *ptr;
		hi2 = *(ptr + 8);
	} while (hi1 != hi2);

	return (uint64_t)hi1 << 32 | lo;
}

static void hpet_write(uint16_t offset, uint64_t val)
{
	volatile uint32_t *ptr = hpet_base + offset;
	*(ptr + 8) = val >> 32;
	*ptr = (uint32_t)val;
}

static void hpet_pause(void)
{
	hpet_write(0x10, 0);
}

static void hpet_resume(void)
{
	hpet_write(0x10, 1);
}

void hpet_init(void)
{
	ACPI_STATUS r;
	ACPI_TABLE_HPET *tbl;
	uint64_t gencap;
	int i, num;

	r = AcpiGetTable("HPET", 1, (ACPI_TABLE_HEADER **) & tbl);
	if (r != AE_OK) {
		printf("No ACPI HPET table found");
		return;
	}

	hpet_base = kvmap(tbl->Address.Address, HPET_SIZE);
	gencap = *(uint64_t *)hpet_base;
	period_femto = gencap >> 32;
	num = 1 + (gencap >> 8) & 0xf;
	dprintf("HPET:\n");
	dprintf("\tfound at %"PRIx64", mapped at %p\n",
		tbl->Address.Address, hpet_base);
	dprintf("\tperiod:  %-20lx\n", period_femto);
	dprintf("\tcounters:%-20lx\n", num);


	hpet_pause(); /* Stop, in case it's running */

	/* Disable interrupts on all counters. */
	for (i = 0; i < num; i++) {
		hpet_write((0x20 * i) + 0x100, 0);
	}

	hpet_resume(); /* Enable HPET */
}

uint64_t timer_readcounter(void)
{
	uint64_t cnt = hpet_read(0xf0);
	uint64_t nsec = (cnt * period_femto)/1000000L;

	return nsec;
}
