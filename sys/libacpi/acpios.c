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


#include <sys/types.h>
#include <stdint.h>
#include <machine/vmparam.h>
#include <stdlib.h>
#include <microkernel.h>
#include <sys/queue.h>
#include <mrg.h>
#include "acpi.h"

#if 0
#define dbgprintf(...) printf(__VA_ARGS__)
#else
#define dbgprintf(...)
#endif

ACPI_PHYSICAL_ADDRESS acpi_rdsp = -1;
static int acpi_pltfd = -1;

struct acpi_intr {
	ACPI_OSD_HANDLER hdlr;
	void *ctx;
	 SLIST_ENTRY(acpi_entry) list;
};
SLIST_HEAD(, acpi_intr) acpi_intrs = SLIST_HEAD_INITIALIZER(&acpi_intrs);

static void acpi_interrupt_handler(int intr, void *opq)
{
	uint32_t x;
	struct acpi_intr *ai = (struct acpi_intr *) opq;

	dbgprintf("ACPI Interrupt handler #%d\n", intr);
	x = ai->hdlr(ai->ctx);
	dbgprintf("Handler returned %d\n", x);
}

#define acpi_init()					\
	do {						\
		if (acpi_pltfd < 0)			\
			acpi_pltfd = sys_open(0);	\
		if (acpi_pltfd < 0)			\
			return AE_ERROR;		\
	} while(0)

/*
 * OSL Initialization and shutdown primitives
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsInitialize
ACPI_STATUS AcpiOsInitialize(void)
{

	dbgprintf("%s called\n", __FUNCTION__);
	acpi_init();
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsTerminate
ACPI_STATUS AcpiOsTerminate(void)
{

	dbgprintf("%s called\n", __FUNCTION__);
	if (acpi_pltfd >= 0)
		sys_close(acpi_pltfd);
	return AE_OK;
}
#endif



#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetRootPointer
ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void)
{

	dbgprintf("%s called (rptr: %lx)\n", __FUNCTION__, acpi_rdsp);
	if (acpi_rdsp == -1) {
		if (AcpiFindRootPointer(&acpi_rdsp) != AE_OK)
			return AE_ERROR;
	}
	return acpi_rdsp;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsPredefinedOverride
ACPI_STATUS
AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES * InitVal,
			 ACPI_STRING * NewVal)
{

	*NewVal = NULL;
	dbgprintf("%s called\n", __FUNCTION__);
	return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsTableOverride
ACPI_STATUS
AcpiOsTableOverride(ACPI_TABLE_HEADER * ExistingTable,
		    ACPI_TABLE_HEADER ** NewTable)
{

	*NewTable = NULL;
	dbgprintf("%s called\n", __FUNCTION__);
	return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsPhysicalTableOverride
ACPI_STATUS
AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER * ExistingTable,
			    ACPI_PHYSICAL_ADDRESS * NewAddress,
			    UINT32 * NewTableLength)
{

	*NewAddress = NULL;
	*NewTableLength = 0;
	dbgprintf("%s called\n", __FUNCTION__);
	return AE_OK;
}
#endif



void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS pa, ACPI_SIZE len)
{
	int ret;
	vaddr_t va;
	unsigned i, pages;

	pages = (len + PAGE_SIZE - 1) >> PAGE_SHIFT;

	dbgprintf("requesting to map %lx(%d)\n", pa, len);
	acpi_init();
	va = vmap_alloc(len, VFNT_MMIO);
	if (va == 0)
		return NULL;
	for (i = 0; i < pages; i++) {
		ret = sys_iomap(acpi_pltfd,
				va + (i << PAGE_SHIFT),
				trunc_page(pa) + (i << PAGE_SHIFT));
		if (ret < 0) {
			int j;
			for (j = 0; j < i; j++)
				sys_iounmap(acpi_pltfd,
					    va + (j << PAGE_SHIFT));
			return NULL;
		}
	}
	dbgprintf("%lx = %s(%lx, %ld) called\n", va, __FUNCTION__, pa,
		  len);
	return (void *) (uintptr_t) (va + (pa & PAGE_MASK));
}

void AcpiOsUnmapMemory(void *ptr, ACPI_SIZE len)
{
	vaddr_t va;
	unsigned i, pages;

	dbgprintf("unmapping %p(%d)\n", ptr, len);
	acpi_init();
	va = (vaddr_t) (uintptr_t) ptr;
	pages = (len + PAGE_SIZE - 1) >> PAGE_SHIFT;
	for (i = 0; i < pages; i++)
		sys_iounmap(acpi_pltfd, va + (i << PAGE_SHIFT));
	vmap_free(trunc_page(va), len);
}


void *AcpiOsAllocate(ACPI_SIZE size)
{
	void *ptr;

	ptr = malloc(size);
	memset(ptr, 0, size);
	return ptr;
}

void AcpiOsFree(void *ptr)
{

	free(ptr);
}

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCreateCache
ACPI_STATUS
AcpiOsCreateCache(const char *CacheName,
		  UINT16 ObjectSize,
		  UINT16 MaxDepth, ACPI_CACHE_T ** ReturnCache)
{

	dbgprintf("(%s called: %s[%d]\n", __FUNCTION__, CacheName,
		  ObjectSize);
	*ReturnCache = (ACPI_CACHE_T *) (uintptr_t) ObjectSize;
	return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsPurgeCache
ACPI_STATUS AcpiOsPurgeCache(ACPI_CACHE_T * Cache)
{

	dbgprintf("(%s called.)\n", __FUNCTION__);
	return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsDeleteCache
ACPI_STATUS AcpiOsDeleteCache(ACPI_CACHE_T * Cache)
{

	dbgprintf("(%s called.)\n", __FUNCTION__);
	return AE_OK;
}
#endif


#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAcquireObject
void *AcpiOsAcquireObject(ACPI_CACHE_T * Cache)
{
	void *ptr;
	size_t size = (size_t) (uintptr_t) Cache;

	ptr = malloc(size);
	memset(ptr, 0, size);
	return ptr;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReleaseObject
ACPI_STATUS AcpiOsReleaseObject(ACPI_CACHE_T * Cache, void *Object)
{

	free(Object);
	return AE_OK;
}
#endif


/*
 * Semaphore primitives
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCreateSemaphore
ACPI_STATUS
AcpiOsCreateSemaphore(UINT32 MaxUnits,
		      UINT32 InitialUnits, ACPI_SEMAPHORE * OutHandle)
{

	dbgprintf("%s called\n", __FUNCTION__);
	return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsDeleteSemaphore
ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle)
{

	dbgprintf("%s called\n", __FUNCTION__);
	return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWaitSemaphore
ACPI_STATUS
AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout)
{

	return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsSignalSemaphore
ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units)
{

	return AE_OK;
}
#endif

/*
 * Spinlock primitives
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCreateLock
ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK * OutHandle)
{

	dbgprintf("%s called\n", __FUNCTION__);
	return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsDeleteLock
void AcpiOsDeleteLock(ACPI_SPINLOCK Handle)
{

	dbgprintf("%s called\n", __FUNCTION__);
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAcquireLock
ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle)
{

	return 0;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReleaseLock
void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags)
{

}
#endif

/*
 * Platform and hardware-independent I/O interfaces
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReadPort
ACPI_STATUS
AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 * Value, UINT32 Width)
{
	unsigned long port;
	uint64_t val;

	switch (Width) {
	case 8:
		port = IOPORT_BYTE(Address);
		break;
	case 16:
		port = IOPORT_WORD(Address);
		break;
	case 32:
		port = IOPORT_DWORD(Address);
		break;
	default:
		return AE_ERROR;
	}

	dbgprintf("%s called\n", __FUNCTION__);
	sys_in(acpi_pltfd, port, &val);
	*Value = (uint32_t) val;
	return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWritePort
ACPI_STATUS
AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width)
{
	unsigned long port;
	uint64_t val;

	dbgprintf("%s called (%d)\n", __FUNCTION__, Width);
	switch (Width) {
	case 8:
		port = IOPORT_BYTE(Address);
		break;
	case 16:
		port = IOPORT_WORD(Address);
		break;
	case 32:
		port = IOPORT_DWORD(Address);
		break;
	default:
		return AE_ERROR;
	}

	sys_out(acpi_pltfd, port, (uint64_t) Value);
	return AE_OK;
}
#endif

/*
 * Platform and hardware-independent physical memory interfaces
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReadMemory
ACPI_STATUS
AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address,
		 UINT64 * Value, UINT32 Width)
{

	dbgprintf("%s called\n", __FUNCTION__);
	return AE_NO_HANDLER;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWriteMemory
ACPI_STATUS
AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address,
		  UINT64 Value, UINT32 Width)
{

	dbgprintf("%s called\n", __FUNCTION__);
	return AE_NO_HANDLER;
}
#endif


/*
 * Platform and hardware-independent PCI configuration space access
 * Note: Can't use "Register" as a parameter, changed to "Reg" --
 * certain compilers complain.
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReadPciConfiguration
#define PCI_CFG_ADDR 0xcf8
#define PCI_CFG_DATA 0xcfc
ACPI_STATUS
AcpiOsReadPciConfiguration(ACPI_PCI_ID * PciId,
			   UINT32 Reg, UINT64 * Value, UINT32 Width)
{
	int ret;
	uint32_t inaddr = 0xcfc + (Reg & 3);
	uint32_t outval = (PciId->Bus << 16)
		| (PciId->Device << 11)
		| (PciId->Function << 8)
		| (Reg & ~0x3)
		| 0x80000000; /* enable bit */
	uint64_t port;

	ret = sys_out(acpi_pltfd, IOPORT_DWORD(0xcf8), outval);
	if (ret)
		return -1;
	switch (Width) {
	case 32: 
		port = IOPORT_DWORD(inaddr);
		break;
	case 16:
		port = IOPORT_WORD(inaddr);
		break;
	case 8:
		port = IOPORT_BYTE(inaddr);
		break;
	default:
		return -1;
	}
	sys_in(acpi_pltfd, port, Value);
	return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWritePciConfiguration
ACPI_STATUS
AcpiOsWritePciConfiguration(ACPI_PCI_ID * PciId,
			    UINT32 Reg, UINT64 Value, UINT32 Width)
{
	int ret;
	uint32_t inaddr = 0xcfc + (Reg & 3);
	uint32_t outval = (PciId->Bus << 16)
		| (PciId->Device << 11)
		| (PciId->Function << 8)
		| (Reg & ~0x3)
		| 0x80000000; /* enable bit */
	uint64_t port;

	ret = sys_out(acpi_pltfd, IOPORT_DWORD(0xcf8), outval);
	if (ret)
		return -1;
	switch (Width) {
	case 32: 
		port = IOPORT_DWORD(inaddr);
		break;
	case 16:
		port = IOPORT_WORD(inaddr);
		break;
	case 8:
		port = IOPORT_BYTE(inaddr);
		break;
	default:
		return -1;
	}
	sys_out(acpi_pltfd, port, Value);
	return AE_OK;
}
#endif

/*
 * Miscellaneous
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetTimer
UINT64 AcpiOsGetTimer(void)
{

	dbgprintf("%s called\n", __FUNCTION__);
	asm volatile ("ud2\n");
	return AE_NO_HANDLER;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsSignal
ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info)
{

	dbgprintf("%s called\n", __FUNCTION__);
	asm volatile ("ud2\n");
	return AE_NO_HANDLER;
}
#endif


/*
 * Threads and Scheduling
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetThreadId
ACPI_THREAD_ID AcpiOsGetThreadId(void)
{

	return 1;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsExecute
ACPI_STATUS
AcpiOsExecute(ACPI_EXECUTE_TYPE Type,
	      ACPI_OSD_EXEC_CALLBACK Function, void *Context)
{

	dbgprintf("%s called\n", __FUNCTION__);
	asm volatile ("ud2\n");
	return AE_NO_HANDLER;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWaitEventsComplete
void AcpiOsWaitEventsComplete(void)
{

	dbgprintf("%s called\n", __FUNCTION__);
	asm volatile ("ud2\n");
	asm volatile ("ud2\n");
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsSleep
void AcpiOsSleep(UINT64 Milliseconds)
{

	dbgprintf("%s called\n", __FUNCTION__);
	asm volatile ("ud2\n");
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsStall
void AcpiOsStall(UINT32 Microseconds)
{

	dbgprintf("%s called\n", __FUNCTION__);
	asm volatile ("ud2\n");
}
#endif

/*
 * Interrupt handlers
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsInstallInterruptHandler
ACPI_STATUS
AcpiOsInstallInterruptHandler(UINT32 InterruptNumber,
			      ACPI_OSD_HANDLER ServiceRoutine,
			      void *Context)
{
	struct acpi_intr *ai;
	int intr, rc;

	dbgprintf("%s called\n", __FUNCTION__);
	ai = malloc(sizeof(*ai));
	if (ai == NULL)
		return AE_NO_MEMORY;

	intr = intalloc();
	rc = sys_mapirq(acpi_pltfd, InterruptNumber, intr);
	if (rc != 0) {
		free(ai);
		intfree(intr);
		return AE_NOT_ACQUIRED;
	}

	ai->hdlr = ServiceRoutine;
	ai->ctx = Context;
	SLIST_INSERT_HEAD(&acpi_intrs, ai, list);
	inthandler(intr, acpi_interrupt_handler, ai);
	return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsRemoveInterruptHandler
ACPI_STATUS
AcpiOsRemoveInterruptHandler(UINT32 InterruptNumber,
			     ACPI_OSD_HANDLER ServiceRoutine)
{

	dbgprintf("%s called\n", __FUNCTION__);
	asm volatile ("ud2\n");
	return AE_NO_HANDLER;
}
#endif
