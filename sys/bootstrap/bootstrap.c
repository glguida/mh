#include <stdio.h>

/*
 * ACPI platform.
 */

#include <limits.h>
#include <microkernel.h>
#include <acpi/acpi.h>
#include <microkernel.h>
#include <drex/drex.h>
#include <squoze.h>

#define ACPI_BUFFER_SIZE (10*1024)
char *acpi_buffer;


static ACPI_STATUS
acpi_set_int(ACPI_HANDLE handle, const char *path,
	     ACPI_INTEGER val)
{
	ACPI_OBJECT_LIST args;
	ACPI_OBJECT obj;

	if (handle == NULL)
		handle = ACPI_ROOT_OBJECT;

	obj.Type = ACPI_TYPE_INTEGER;
	obj.Integer.Value = val;

	args.Count = 1;
	args.Pointer = &obj;

	return AcpiEvaluateObject(handle, path, &args, NULL);
}

#define HWCREAT_FULL(_c)				\
	((_c->irqsegs + _c->piosegs + _c->memsegs) > SYS_HWCREAT_MAX_SEGMENTS)

static ACPI_STATUS
acpi_per_resource_irq(ACPI_RESOURCE *res, void *ctx)
{
	struct sys_hwcreat_cfg *cfg = (struct sys_hwcreat_cfg *)ctx;

	switch (res->Type) {

	case ACPI_RESOURCE_TYPE_IRQ: {
		ACPI_RESOURCE_IRQ *p = &res->Data.Irq;
		if (!p || !p->InterruptCount || !p->Interrupts[0])
			break;
		
		if (HWCREAT_FULL(cfg))
			return AE_CTRL_TERMINATE;

		printf(",IRQ%d", p->Interrupts[0]);
		cfg->segs[cfg->irqsegs].base = p->Interrupts[0];
		cfg->segs[cfg->irqsegs].len = 1;
		cfg->irqsegs++;
		break;
	}
	case ACPI_RESOURCE_TYPE_EXTENDED_IRQ: {
		ACPI_RESOURCE_EXTENDED_IRQ *p = &res->Data.ExtendedIrq;
		if (!p || !p->InterruptCount || !p->Interrupts[0])
			break;
		
		if (HWCREAT_FULL(cfg))
			return AE_CTRL_TERMINATE;

		printf(",IRQ%d", p->Interrupts[0]);
		cfg->segs[cfg->irqsegs].base = p->Interrupts[0];
		cfg->segs[cfg->irqsegs].len = 1;
		cfg->irqsegs++;
		break;
	}
	default:
		break;
	}

	return AE_OK;
}

static ACPI_STATUS
acpi_per_device(ACPI_HANDLE devhandle, UINT32 level, void *ctx, void **retval)
{
	ACPI_HANDLE *crshandle = NULL;
	ACPI_PNP_DEVICE_ID *devid;
	ACPI_BUFFER buffer;
	ACPI_STATUS status;
	struct sys_hwcreat_cfg hwcreat = { 0, };
	char *name[13], *devidstr;

	/* Add only if there is a _CRS method */
	status = AcpiGetHandle(devhandle, METHOD_NAME__CRS, ACPI_CAST_PTR (ACPI_HANDLE, &crshandle));
	if (ACPI_FAILURE(status))
		return AE_OK;

	buffer.Pointer = name;
	buffer.Length = 13;
	status = AcpiGetName(devhandle, ACPI_SINGLE_NAME, &buffer);
	if (ACPI_FAILURE(status))
		return status;

	AcpiUtExecute_HID(devhandle, &devid);
	devidstr = devid ? devid->String ? devid->String : "" : "";

	hwcreat.nameid = squoze(name);
	hwcreat.vendorid = squoze("ACPI");
	hwcreat.deviceid = squoze(devidstr);

	
	printf("Adding device %s [ACPI,%s", name, devidstr);

	status = AcpiWalkResources(devhandle, METHOD_NAME__CRS,
				   acpi_per_resource_irq, &hwcreat);
	if (ACPI_FAILURE(status)) {
		AcpiOsPrintf("AcpiWalkResources failed: %s\n",
			     AcpiFormatException(status));
		return status;
	}
	
	printf("]\n");

	/* Add device to system bus */
	printf("creat() = %d\n", sys_hwcreat(&hwcreat, 0111));
}

int
platform_init(void)
{
	ACPI_STATUS as;
	unsigned int ptr;

	/*
	 * Initialize ACPI
	 */

	as = AcpiFindRootPointer(&ptr);
	if (ACPI_FAILURE(as))
		return -1;
	printf("ACPI ROOT: %x\n", ptr);

	as = AcpiInitializeTables(NULL, 0, FALSE);
	if (ACPI_FAILURE(as))
		return -2;

	as = AcpiInitializeSubsystem();
	if (ACPI_FAILURE(as))
		return -4;

	as = AcpiLoadTables();
	if (ACPI_FAILURE(as))
		return -3;

	as = AcpiInitializeObjects(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(as))
		return -5;

	printf("ACPI initialized and loaded. Enabling.");

	as = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(as))
		return -6;


	as = acpi_set_int(NULL, "_PIC", 1);
	if (ACPI_FAILURE(as) && (as != AE_NOT_FOUND))
		return -7;

	/*
	 * Scan ACPI devices.
	 */

	printf("ACPI Platform devices:\n");
	AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT, INT_MAX,
			  NULL, acpi_per_device, NULL, NULL);

	return 0;
}

int main()
{
	int ret;

	printf("MRG bootstrap initiated.\n");

	ret = platform_init();
	if (ret < 0) {
		printf("platform initialization failed: %d\n", ret);
		return -1;
	}

	printf("Successfully initializated");
	lwt_sleep();
}
