#include <uk/types.h>
#include <uk/param.h>
#include <uk/locks.h>
#include "../ukern/heap.h"
#include "../ukern/structs.h"
#include "../ukern/vmap.h"
#include "./acpica.h"

#include "acpi.h"
extern ACPI_PHYSICAL_ADDRESS acpi_rdsp;



/*
 * OSL Initialization and shutdown primitives
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsInitialize
ACPI_STATUS
AcpiOsInitialize (
    void)
{

    dbgprintf("%s called\n", __FUNCTION__);
    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsTerminate
ACPI_STATUS
AcpiOsTerminate (
    void)
{

    dbgprintf("%s called\n", __FUNCTION__);
    return AE_OK;
}
#endif



#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetRootPointer
ACPI_PHYSICAL_ADDRESS
AcpiOsGetRootPointer (
		      void)
{

    dbgprintf("%s called (rptr: %08lx)\n", __FUNCTION__, acpi_rdsp);
    return acpi_rdsp;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsPredefinedOverride
ACPI_STATUS
AcpiOsPredefinedOverride (
			  const ACPI_PREDEFINED_NAMES *InitVal,
			  ACPI_STRING                 *NewVal)
{

    dbgprintf("%s called\n", __FUNCTION__);
    return AE_NOT_CONFIGURED;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsTableOverride
ACPI_STATUS
AcpiOsTableOverride (
		     ACPI_TABLE_HEADER       *ExistingTable,
		     ACPI_TABLE_HEADER       **NewTable)
{

    dbgprintf("%s called\n", __FUNCTION__);
    return AE_NO_ACPI_TABLES;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsPhysicalTableOverride
ACPI_STATUS
AcpiOsPhysicalTableOverride (
			     ACPI_TABLE_HEADER       *ExistingTable,
			     ACPI_PHYSICAL_ADDRESS   *NewAddress,
			     UINT32                  *NewTableLength)
{

    dbgprintf("%s called\n", __FUNCTION__);
    return AE_NO_ACPI_TABLES;
}
#endif



void *
AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS pa, ACPI_SIZE len)
{
    void *ptr;

    dbgprintf("requesting to map %lx(%d)\n", pa, len);
    ptr = kvmap(pa, len);
    dbgprintf("%p = %s(%lx, %ld) called\n", ptr, __FUNCTION__, pa, len);
    return (void *)(uintptr_t)ptr;
}

void
AcpiOsUnmapMemory (
    void                    *ptr,
    ACPI_SIZE               len)
{

    dbgprintf("unmapping %p(%d)\n", ptr, len);
    kvunmap((vaddr_t)ptr, len);
}


void *
AcpiOsAllocate(ACPI_SIZE size)
{
    void *ptr;

    ptr = heap_alloc(size);
    dbgprintf("%s(%d) = %p\n", __FUNCTION__, size, ptr);
    return ptr;
}

void
AcpiOsFree(void *ptr)
{

    dbgprintf("%s(%p)\n", __FUNCTION__, ptr);
    heap_free(ptr);
}

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCreateCache
ACPI_STATUS
AcpiOsCreateCache (
		   const char              *CacheName,
		   UINT16                  ObjectSize,
		   UINT16                  MaxDepth,
		   ACPI_CACHE_T            **ReturnCache)
{
    struct slab *sc;

    sc = heap_alloc(sizeof(struct slab));
    if (sc == NULL)
	return AE_NO_MEMORY;

    if (structs_register(sc, (char *)CacheName, ObjectSize, NULL, 1))
	return AE_NO_MEMORY;

    *ReturnCache = (void *)sc;
    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsPurgeCache
ACPI_STATUS
AcpiOsPurgeCache (
    ACPI_CACHE_T            *Cache)
{

    dbgprintf("(%s called.)\n", __FUNCTION__);
    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsDeleteCache
ACPI_STATUS
AcpiOsDeleteCache (
		   ACPI_CACHE_T            *Cache)
{

    structs_deregister((struct slab *)Cache);
    return AE_OK;
}
#endif


#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAcquireObject
void *
AcpiOsAcquireObject (
    ACPI_CACHE_T            *Cache)
{

    return structs_alloc((struct slab *)Cache);
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReleaseObject
ACPI_STATUS
AcpiOsReleaseObject (
    ACPI_CACHE_T            *Cache,
    void                    *Object)
{

    structs_free(Object);
    return AE_OK;
}
#endif


/*
 * Semaphore primitives
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCreateSemaphore
ACPI_STATUS
AcpiOsCreateSemaphore (
		       UINT32                  MaxUnits,
		       UINT32                  InitialUnits,
		       ACPI_SEMAPHORE          *OutHandle)
{

    asm volatile("ud2\n");
    return AE_NO_HANDLER;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsDeleteSemaphore
ACPI_STATUS
AcpiOsDeleteSemaphore (
		       ACPI_SEMAPHORE          Handle)
{
    asm volatile("ud2\n");
    return AE_NO_HANDLER;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWaitSemaphore
ACPI_STATUS
AcpiOsWaitSemaphore (
		     ACPI_SEMAPHORE          Handle,
		     UINT32                  Units,
		     UINT16                  Timeout)
{
    asm volatile("ud2\n");
    return AE_NO_HANDLER;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsSignalSemaphore
ACPI_STATUS
AcpiOsSignalSemaphore (
		       ACPI_SEMAPHORE          Handle,
		       UINT32                  Units)
{
    asm volatile("ud2\n");
    return AE_NO_HANDLER;
}
#endif

/*
 * Spinlock primitives
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsCreateLock
ACPI_STATUS
AcpiOsCreateLock (
		  ACPI_SPINLOCK           *OutHandle)
{
    lock_t *l;

    l = (lock_t *)heap_alloc(sizeof(lock_t));
    if (l == NULL)
	return AE_NO_MEMORY;

    *l = 0;
    *OutHandle = l;
    return AE_OK;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsDeleteLock
void
AcpiOsDeleteLock (
		  ACPI_SPINLOCK           Handle)
{

    heap_free(Handle);
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsAcquireLock
ACPI_CPU_FLAGS
AcpiOsAcquireLock (
		   ACPI_SPINLOCK           Handle)
{

    /* XXX: IRQ in flags? */
    spinlock(Handle);
    return 0;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReleaseLock
void
AcpiOsReleaseLock (
		   ACPI_SPINLOCK           Handle,
		   ACPI_CPU_FLAGS          Flags)
{

    /* XXX: IRQ in flags? */
    spinunlock(Handle);
}
#endif

/*
 * Platform and hardware-independent I/O interfaces
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReadPort
ACPI_STATUS
AcpiOsReadPort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  *Value,
    UINT32                  Width)
{

    asm volatile("ud2\n");
    return AE_NO_HANDLER;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWritePort
ACPI_STATUS
AcpiOsWritePort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  Value,
    UINT32                  Width)
{

    asm volatile("ud2\n");
    return AE_NO_HANDLER;
}
#endif

/*
 * Platform and hardware-independent physical memory interfaces
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReadMemory
ACPI_STATUS
AcpiOsReadMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  *Value,
    UINT32                  Width)
{

    asm volatile("ud2\n");
    return AE_NO_HANDLER;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWriteMemory
ACPI_STATUS
AcpiOsWriteMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  Value,
    UINT32                  Width) 
{

    asm volatile("ud2\n");
    return AE_NO_HANDLER;
}
#endif


/*
 * Platform and hardware-independent PCI configuration space access
 * Note: Can't use "Register" as a parameter, changed to "Reg" --
 * certain compilers complain.
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsReadPciConfiguration
ACPI_STATUS
AcpiOsReadPciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  *Value,
    UINT32                  Width)
{

    asm volatile("ud2\n");
    return AE_NO_HANDLER;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWritePciConfiguration
ACPI_STATUS
AcpiOsWritePciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  Value,
    UINT32                  Width)
{

    asm volatile("ud2\n");
    return AE_NO_HANDLER;
}
#endif

/*
 * Miscellaneous
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetTimer
UINT64
AcpiOsGetTimer (
    void)
{

    asm volatile("ud2\n");
    return AE_NO_HANDLER;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsSignal
ACPI_STATUS
AcpiOsSignal (
    UINT32                  Function,
    void                    *Info)
{

    asm volatile("ud2\n");
    return AE_NO_HANDLER;
}
#endif


/*
 * Threads and Scheduling
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsGetThreadId
ACPI_THREAD_ID
AcpiOsGetThreadId (
		   void)
{

    asm volatile("ud2\n");
    return 0;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsExecute
ACPI_STATUS
AcpiOsExecute (
    ACPI_EXECUTE_TYPE       Type,
    ACPI_OSD_EXEC_CALLBACK  Function,
    void                    *Context)
{

    asm volatile("ud2\n");
    return AE_NO_HANDLER;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsWaitEventsComplete
void
AcpiOsWaitEventsComplete (
    void)
{

    asm volatile("ud2\n");
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsSleep
void
AcpiOsSleep (
    UINT64                  Milliseconds)
{

    asm volatile("ud2\n");
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsStall
void
AcpiOsStall (
    UINT32                  Microseconds)
{

    asm volatile("ud2\n");
}
#endif

/*
 * Interrupt handlers
 */
#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsInstallInterruptHandler
ACPI_STATUS
AcpiOsInstallInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine,
    void                    *Context)
{

    asm volatile("ud2\n");
    return AE_NO_HANDLER;
}
#endif

#ifndef ACPI_USE_ALTERNATE_PROTOTYPE_AcpiOsRemoveInterruptHandler
ACPI_STATUS
AcpiOsRemoveInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine)
{

    asm volatile("ud2\n");
    return AE_NO_HANDLER;
}
#endif

