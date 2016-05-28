/*
 * ACPI platform.
 */

#include <inttypes.h>
#include <limits.h>
#include <microkernel.h>
#include <acpi/acpi.h>
#include <acpi/accommon.h>
#include <squoze.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include "internal.h"

#define ACPI_SCAN_MAX_LEVELS 16
#define ACPI_BUFFER_SIZE (10*1024)
char *acpi_buffer;

/* PCI  Device.  Get name  and  EISAIDs  and then  get
 * information from PCI config BARs. */

#define PCI_VENDOR(reg)		((reg) & 0xFFFF)
#define PCI_DEVICE(reg)		(((reg) >> 16) & 0xFFFF)
#define PCI_CLASS(reg)		((reg) >> 8)
#define PCI_HDRTYPE(reg)	(((reg) >> 16) & 0xFF)
#define PCI_BRIDGE_IOBASE(reg)  ((reg) & 0xFF)
#define PCI_BRIDGE_IOLIMIT(reg)  (((reg) >> 8) & 0xFF)
#define PCI_BRIDGE_IOBASE_UPPER(reg) ((reg) & 0xFFFF)
#define PCI_BRIDGE_IOLIMIT_UPPER(reg) (((reg) >> 16) & 0xFFFF)
#define PCI_BRIDGE_MEMBASE(reg) ((reg) & 0xFFFF)
#define PCI_BRIDGE_MEMLIMIT(reg) (((reg) >> 16) & 0xFFFF)
#define PCI_BRIDGE_PMEMBASE(reg) ((reg) & 0xFFFF)
#define PCI_BRIDGE_PMEMLIMIT(reg) (((reg) >> 16) & 0xFFFF)
#define PCI_BRIDGE_SECONDBUS(reg) (((reg) >> 8) & 0xff)

#define PCI_VENDOR_REG		0x00
#define PCI_CLASS_REG 		0x08
#define PCI_HDRTYPE_REG		0x0c
#define PCI_BAR_REG(_x)		(0x10 + (_x) * 4)
#define PCI_BRIDGE_BUSNO_REG    0x18
#define PCI_BRIDGE_IO_REG	0x1c
#define PCI_BRIDGE_MEM_REG	0x20
#define PCI_BRIDGE_PMEM_REG	0x24
#define PCI_BRIDGE_PMEM_BASEUPPER_REG 0x28
#define PCI_BRIDGE_PMEM_LIMITUPPER_REG 0x2c
#define PCI_BRIDGE_IOUPPER_REG   0x30


/*
 * ACPI devices scan.
 *
 * This part scans for devices not enumerated by their buses and
 * handled by ACPI. All devices that have a _CRS method are selected
 * and registered in the kernel.
 */

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

#define HWCREAT_SEGS_FULL(_c)				\
	(((_c)->nirqsegs + (_c)->npiosegs) > SYS_HWCREAT_MAX_SEGMENTS)

#define HWCREAT_MEMSEGS_FULL(_c)		\
	((_c)->nmemsegs > SYS_HWCREAT_MAX_SEGMENTS)


static ACPI_STATUS
acpi_per_resource_irq(ACPI_RESOURCE *res, void *ctx)
{
	struct sys_hwcreat_cfg *cfg = (struct sys_hwcreat_cfg *)ctx;
	uint16_t irq;

	switch (res->Type) {
	case ACPI_RESOURCE_TYPE_IRQ: {
		ACPI_RESOURCE_IRQ *p = &res->Data.Irq;

		if (!p || !p->InterruptCount || !p->Interrupts[0])
			return AE_OK;

		irq = p->Interrupts[0];
		break;
	}
	case ACPI_RESOURCE_TYPE_EXTENDED_IRQ: {
		ACPI_RESOURCE_EXTENDED_IRQ *p = &res->Data.ExtendedIrq;

		if (!p || !p->InterruptCount || !p->Interrupts[0])
			return AE_OK;

		irq = p->Interrupts[0];
		break;
	}
	default:
		return AE_OK;
	}
		
	if (HWCREAT_SEGS_FULL(cfg))
		return AE_CTRL_TERMINATE;

	printf(",IRQ%d", irq);
	cfg->segs[cfg->nirqsegs].base = irq;
	cfg->segs[cfg->nirqsegs].len = 1;
	cfg->nirqsegs++;
	return AE_OK;
}

static ACPI_STATUS
acpi_per_resource_pio(ACPI_RESOURCE *res, void *ctx)
{
	struct sys_hwcreat_cfg *cfg = (struct sys_hwcreat_cfg *)ctx;
	uint16_t start, len;

	switch (res->Type) {
	case ACPI_RESOURCE_TYPE_IO: {
		ACPI_RESOURCE_IO *p = &res->Data.Io;
		if (!p || !p->Minimum || (!p->AddressLength && !p->Maximum))
			return AE_OK;

		start = p->Minimum;
		if (p->AddressLength)
			len = p->AddressLength;
		else
			len = p->Maximum - p->Minimum;

		break;
	}
	case ACPI_RESOURCE_TYPE_FIXED_IO: {
		ACPI_RESOURCE_FIXED_IO *p = &res->Data.FixedIo;
		if (!p || !p->Address || !p->AddressLength)
			return AE_OK;

		start = p->Address;
		len = p->AddressLength;
		break;
	}
	default:
		return AE_OK;
	}

	if (HWCREAT_SEGS_FULL(cfg)) {
		printf(",OVF");
		return AE_CTRL_TERMINATE;
	}

	printf(",IO%02x", start);
	cfg->segs[cfg->nirqsegs + cfg->npiosegs].base = start;
	cfg->segs[cfg->nirqsegs + cfg->npiosegs].len = len;
	cfg->npiosegs++;
	return AE_OK;
}

static ACPI_STATUS
acpi_per_resource_mem(ACPI_RESOURCE *res, void *ctx)
{
	struct sys_hwcreat_cfg *cfg = (struct sys_hwcreat_cfg *)ctx;
	uint64_t start, len;

	switch (res->Type) {
	case ACPI_RESOURCE_TYPE_MEMORY24: {
		ACPI_RESOURCE_MEMORY24 *p = &res->Data.Memory24;
		if (!p || !p->Minimum || (!p->AddressLength && !p->Maximum))
			return AE_OK;

		start = p->Minimum;
		if (p->AddressLength)
			len = p->AddressLength;
		else
			len = p->Maximum - start;
		break;
	}
	case ACPI_RESOURCE_TYPE_MEMORY32: {
		ACPI_RESOURCE_MEMORY32 *p = &res->Data.Memory32;
		if (!p || !p->Minimum || (!p->AddressLength && !p->Maximum))
			return AE_OK;

		start = p->Minimum;
		if (p->AddressLength)
			len = p->AddressLength;
		else
			len = p->Maximum - start;
		break;
	}
	case ACPI_RESOURCE_TYPE_FIXED_MEMORY32: {
		ACPI_RESOURCE_FIXED_MEMORY32 *p = &res->Data.FixedMemory32;
		if (!p  || !p->Address || !p->AddressLength)
			return AE_OK;

		start = p->Address;
		len = p->AddressLength;
		break;
	}
	case ACPI_RESOURCE_TYPE_ADDRESS16: {
		ACPI_RESOURCE_ADDRESS16 *p = &res->Data.Address16;
		if (!p || !p->Minimum || (!p->AddressLength && !p->Maximum))
			return AE_OK;

		start = p->Minimum;
		if (p->AddressLength)
			len = p->AddressLength;
		else
			len = p->Maximum - start;
		break;
	}
	case ACPI_RESOURCE_TYPE_ADDRESS32: {
		ACPI_RESOURCE_ADDRESS32 *p = &res->Data.Address32;
		if (!p || !p->Minimum || (!p->AddressLength && !p->Maximum))
			return AE_OK;

		start = p->Minimum;
		if (p->AddressLength)
			len = p->AddressLength;
		else
			len = p->Maximum - start;
		break;
	}
	case ACPI_RESOURCE_TYPE_ADDRESS64: {
		ACPI_RESOURCE_ADDRESS64 *p = &res->Data.Address64;
		if (!p || !p->Minimum || (!p->AddressLength && !p->Maximum))
			return AE_OK;

		start = p->Minimum;
		if (p->AddressLength)
			len = p->AddressLength;
		else
			len = p->Maximum - start;
		break;
	}
	default:
		return AE_OK;
	}

	if (HWCREAT_MEMSEGS_FULL(cfg)) {
		printf(",OVF");
		return AE_CTRL_TERMINATE;
	}

	printf(",MEM%08l"PRIx64"-%08l"PRIx64, start, start + len);
	cfg->memsegs[cfg->nmemsegs].base = start;
	cfg->memsegs[cfg->nmemsegs].len = len;
	cfg->nmemsegs++;
	return AE_OK;
}

static ACPI_STATUS
acpi_per_device(ACPI_HANDLE devhandle, UINT32 level, void *ctx, void **retval)
{
	static uint64_t pci_pnpid = 0;
	int ret, i, dids;
	struct sys_hwcreat_cfg hwcreat = { 0, };
	ACPI_HANDLE crshandle = NULL;
	ACPI_PNP_DEVICE_ID *devid;
	ACPI_DEVICE_INFO *info;
	ACPI_BUFFER buffer;
	ACPI_STATUS status;
	char name[13], *devidstr;

	if (pci_pnpid == 0)
		pci_pnpid = squoze("PNP0A03");

	if (level >= ACPI_SCAN_MAX_LEVELS) {
		printf("Max level reached in ACPI scan.");
		return AE_OK;
	}

	status = AcpiGetHandle(devhandle, METHOD_NAME__CRS,
			       ACPI_CAST_PTR (ACPI_HANDLE, &crshandle));
	if (ACPI_FAILURE(status))
		return AE_OK;

	buffer.Pointer = name;
	buffer.Length = 13;
	status = AcpiGetName(devhandle, ACPI_SINGLE_NAME, &buffer);
	if (ACPI_FAILURE(status)) {
		printf("AcpiGetName failed: %s",
		       AcpiFormatException(status));
		return status;
	}

	status = AcpiGetObjectInfo (devhandle, &info);
	if (ACPI_FAILURE (status)) {
		printf("AcpiGetObjectInfo failed: %s",
		       AcpiFormatException(status));
		return AE_OK;
	}

	devidstr = info->Valid & ACPI_VALID_HID ? info->HardwareId.String : "";

	hwcreat.nameid = squoze(name);
	hwcreat.vendorid = squoze("ACPI");

	printf("\t%s [ACPI,%s", name, devidstr);

	hwcreat.deviceids[0] = squoze(devidstr);
	for (i = 0; i < MIN(info->CompatibleIdList.Count, SYS_HWCREAT_MAX_DEVIDS - 1); i++) {
		printf(":%s", info->CompatibleIdList.Ids[i].String);
		hwcreat.deviceids[1 + i] = squoze(info->CompatibleIdList.Ids[i].String);
	}

	status = AcpiWalkResources(devhandle, METHOD_NAME__CRS,
				   acpi_per_resource_irq, &hwcreat);
	if (ACPI_FAILURE(status)) {
		AcpiOsPrintf("AcpiWalkResources failed: %s\n",
			     AcpiFormatException(status));
		return AE_OK;
	}

	status = AcpiWalkResources(devhandle, METHOD_NAME__CRS,
				   acpi_per_resource_pio, &hwcreat);
	if (ACPI_FAILURE(status)) {
		AcpiOsPrintf("AcpiWalkResources failed: %s\n",
			     AcpiFormatException(status));
		return AE_OK;
	}

	status = AcpiWalkResources(devhandle, METHOD_NAME__CRS,
				   acpi_per_resource_mem, &hwcreat);
	if (ACPI_FAILURE(status)) {
		AcpiOsPrintf("AcpiWalkResources failed: %s\n",
			     AcpiFormatException(status));
		return AE_OK;;
	}
	printf("]\n");

	/* Add device to system bus */
	devadd(&hwcreat);
	return AE_OK;
}


/*
 * ACPI PCI Scan.
 *
 * The following code scans the PCI root bus and registers devices in
 * the kernel. When a device is found, ACPI is queried for information
 * about the device, specifically for interrupt information and a
 * name, as many devices might be enumerated and given string
 * identifiers by the platform BIOS.
 */

static int
getnfuncs(int bus, int dev)
{
	int ret;
	ACPI_STATUS as;
	ACPI_PCI_ID acpi_pciid;
	uint64_t reg;

	acpi_pciid.Segment = 0;
	acpi_pciid.Bus = bus;
	acpi_pciid.Device = dev;
	acpi_pciid.Function = 0;

	as = AcpiOsReadPciConfiguration(&acpi_pciid, PCI_HDRTYPE_REG, &reg, 32);
	if (ACPI_FAILURE(as))
		return 0;

	return (PCI_HDRTYPE(reg) & 0x80) ? 8 : 1;
}

struct bridgeirqs {
	int ints[4];
};


static int acpi_pci_scanbus(void *root, int bus, struct bridgeirqs *brirqs);

static void
acpi_pci_scandevice(void *root, int bus, int dev, int func, struct bridgeirqs *brirqs)
{
	struct sys_hwcreat_cfg hwcreat;
	ACPI_STATUS as;
	ACPI_PCI_ID acpi_pciid;
	ACPI_DEVICE_INFO *info;
	uint64_t reg;
	void *subdev;
	char name[13];
	int i, j, nbars, hdrtype, ints[4];
	uint64_t nameid = 0;

	acpi_pciid.Segment = 0;
	acpi_pciid.Bus = bus;
	acpi_pciid.Device = dev;
	acpi_pciid.Function = func;

	memset(&hwcreat, 0, sizeof(hwcreat));

	subdev = platform_getpcisubdev(root, dev, func);
	if (subdev != NULL) {
		nameid = platform_getname(subdev);
	}
	if (nameid != 0) {
		unsquozelen(nameid, 13, name);
	} else {
		snprintf(name, 13, "PCI%02x.%02x.%1d",
			 bus, dev, func);
		nameid = squoze(name);
	}
	hwcreat.nameid = nameid;
	hwcreat.vendorid = squoze("ACPI-PCI");

	printf("\t%s [ACPI-PCI", name);

	/* Fill device ids */
	i = 0;
	j = 0;

	AcpiOsReadPciConfiguration(&acpi_pciid, PCI_VENDOR_REG, &reg, 32);
	snprintf(name, 13, "PCI%04x.%04x",
		 (uint16_t)PCI_VENDOR(reg), (uint16_t)PCI_DEVICE(reg));
	hwcreat.deviceids[j++] = squoze(name);

	printf(",%s", name);

	as = AcpiGetObjectInfo(subdev, &info);
	if (ACPI_FAILURE(as))
		goto skip_acpi_dev_info;
	
	if (info->Valid & ACPI_VALID_HID) {
		hwcreat.deviceids[j++] = squoze(info->HardwareId.String);
		printf(":%s", info->HardwareId.String);
	}

	for (i = 0; i < MIN(info->CompatibleIdList.Count, SYS_HWCREAT_MAX_DEVIDS - j); i++) {
		/* only add if it looks like an EISA ID */
		if (info->CompatibleIdList.Ids[i].Length != ACPI_EISAID_STRING_SIZE)
			continue;
		printf(":%s", info->CompatibleIdList.Ids[i].String);
		hwcreat.deviceids[j + i] = squoze(info->CompatibleIdList.Ids[i].String);

	}
  skip_acpi_dev_info:
	if (i < (SYS_HWCREAT_MAX_DEVIDS - j)) {
		AcpiOsReadPciConfiguration(&acpi_pciid, PCI_CLASS_REG, &reg, 32);
		reg = PCI_CLASS(reg);
		snprintf(name, 13, "PCI%02x.%02x.%02x",
			 (unsigned)(reg >> 16) & 0xff,
			 (unsigned)(reg >> 8) & 0xff,
			 (unsigned)(reg & 0xff));
		printf(":%s", name);
		hwcreat.deviceids[j + i] = squoze(name);
	}

	/* Populate IRQs */
	if (brirqs) {
		for (i = 0; i < 4; i++)
			ints[i] = brirqs->ints[(dev + i) % 4];
	} else {
		platform_getpciintrs(root, dev, ints);
	}
	for (i = 0; i < 4; i++) {
		if (ints[i] == 0)
			continue;

		printf(",IRQ%d", ints[i]);
		hwcreat.segs[hwcreat.nirqsegs].base = ints[i];
		hwcreat.segs[hwcreat.nirqsegs].len = 1;
		hwcreat.nirqsegs++;
	}

	AcpiOsReadPciConfiguration(&acpi_pciid, PCI_HDRTYPE_REG, &reg, 32);
	hdrtype = PCI_HDRTYPE(reg) & 0x7f;

	switch (hdrtype) {
	case 0:
		nbars = 6;
		break;
	case 1:
		nbars = 2;
		break;
	default:
		printf("Unsupported HDR TYPE %d\n", hdrtype);
		nbars = 0;
		break;
	}

	/*
	 * Read BARs and populate.
	 */

	/* PIO */
	for (i = 0; i < nbars; i++) {
		uint64_t orig, size = 0;
		uint32_t iobase, iolen;

		reg = 0;
		if (AcpiOsReadPciConfiguration(&acpi_pciid, PCI_BAR_REG(i), &reg, 32))
			continue;

		if (reg == 0)
			continue;

		if ((reg & 1) == 0) {
			/* MEM BAR */
			continue; 
		}

		orig = reg;

		if (AcpiOsWritePciConfiguration(&acpi_pciid, PCI_BAR_REG(i), (uint64_t)-1, 32))
			continue;
		if (AcpiOsReadPciConfiguration(&acpi_pciid, PCI_BAR_REG(i), &size, 32))
			continue;
		if (AcpiOsWritePciConfiguration(&acpi_pciid, PCI_BAR_REG(i), orig, 32))
			continue;
		size &= ~(uint64_t)0x3LL;
		if (size == 0)
			continue;

		iobase = (uint32_t)(orig & ~(uint64_t)0x3LL);
		iolen = 1 << (ffs(size) - 1);

		printf(",IO%04x-%04x", iobase, iolen);
		hwcreat.segs[hwcreat.nirqsegs + hwcreat.npiosegs].base = iobase;
		hwcreat.segs[hwcreat.nirqsegs + hwcreat.npiosegs].len = iolen;
		hwcreat.npiosegs++;
	}

	/* Populate PCI bridge I/O */
	if (hdrtype == 1) {
		int base32, limit32;
		uint32_t iobase, iolimit;

		AcpiOsReadPciConfiguration(&acpi_pciid, PCI_BRIDGE_IO_REG, &reg, 32);
		iobase = (PCI_BRIDGE_IOBASE(reg) & 0xf0) << 8;
		iolimit = 0xfff | ((PCI_BRIDGE_IOLIMIT(reg)& 0xf0) << 8);

		AcpiOsReadPciConfiguration(&acpi_pciid, PCI_BRIDGE_IOUPPER_REG, &reg, 32);
		if ((PCI_BRIDGE_IOBASE(reg) & 0xf) == 1) {
			iobase |= (PCI_BRIDGE_IOBASE_UPPER(reg) << 16);
		}
		if ((PCI_BRIDGE_IOLIMIT(reg) & 0xf) == 1) {
			iolimit |= (PCI_BRIDGE_IOLIMIT_UPPER(reg) << 16);
		}

		if (iobase < iolimit) {
			printf(",IO%08x-%08x", iobase, iolimit);
			hwcreat.segs[hwcreat.nirqsegs + hwcreat.npiosegs].base = iobase;
			hwcreat.segs[hwcreat.nirqsegs + hwcreat.npiosegs].len = iolimit;
			hwcreat.npiosegs++;
		}
	}

	/* MEM */
	for (i = 0; i < nbars; i++) {
		uint64_t orig, size = 0;
		uint64_t membase, memlen;

		reg = 0;
		if (AcpiOsReadPciConfiguration(&acpi_pciid, PCI_BAR_REG(i), &reg, 32))
			continue;

		if (reg == 0)
			continue;

		if ((reg & 1) == 1) {
			/* PIO BAR */
			continue; 
		}

		orig = reg;
		if (AcpiOsWritePciConfiguration(&acpi_pciid, PCI_BAR_REG(i), (uint64_t)-1, 32))
			continue;
		if (AcpiOsReadPciConfiguration(&acpi_pciid, PCI_BAR_REG(i), &size, 32))
			continue;
		if (AcpiOsWritePciConfiguration(&acpi_pciid, PCI_BAR_REG(i), orig, 32))
			continue;
		size &= ~(uint64_t)0xfLL;
		if (size == 0)
			continue;
		
		membase = (uint64_t)(orig & ~(uint64_t)0xfLL);
		memlen = 1 << (ffs(size) - 1);

		printf(",MEM%08x-%08x", membase, memlen);
		hwcreat.memsegs[hwcreat.nmemsegs].base = membase;
		hwcreat.memsegs[hwcreat.nmemsegs].len = memlen;
		hwcreat.nmemsegs++;
	}

	/* Populate PCI bridge MEM */
	if (hdrtype == 1) {
		uint64_t membase, memlimit;

		AcpiOsReadPciConfiguration(&acpi_pciid, PCI_BRIDGE_MEM_REG, &reg, 32);
		membase = (PCI_BRIDGE_MEMBASE(reg) & 0xfff0) << 20;
		memlimit = ((PCI_BRIDGE_MEMLIMIT(reg) & 0xfff0) << 20) + 0xfffff;
		if (membase < memlimit) {
			printf(",MEM%08llx-%08llx", membase, memlimit);
		}


		AcpiOsReadPciConfiguration(&acpi_pciid, PCI_BRIDGE_PMEM_REG, &reg, 32);
		membase = (PCI_BRIDGE_PMEMBASE(reg) & 0xfff0) << 20;
		memlimit = ((PCI_BRIDGE_PMEMLIMIT(reg) & 0xfff0) << 20) + 0xfffff;
		AcpiOsReadPciConfiguration(&acpi_pciid, PCI_BRIDGE_PMEM_BASEUPPER_REG, &reg, 32);
		membase |= (uint64_t)reg << 32;
		memlimit |= (uint64_t)reg << 32;

		if (membase < memlimit) {
			printf(",MEM%08llx-%08llx", membase, memlimit);
			hwcreat.memsegs[hwcreat.nmemsegs].base = membase;
			hwcreat.memsegs[hwcreat.nmemsegs].len = memlimit;
			hwcreat.nmemsegs++;
		}

	}

	printf("]\n");

	/* Add device to system bus */
	devadd(&hwcreat);

	/* If it is a bridge, scan sub-device. */
	if (hdrtype == 1) {
		int subbus;
		struct bridgeirqs brirqs;

		for (i = 0; i < 4; i++)
			brirqs.ints[i] = ints[i];

		AcpiOsReadPciConfiguration(&acpi_pciid, PCI_BRIDGE_BUSNO_REG, &reg, 32);
		subbus = PCI_BRIDGE_SECONDBUS(reg);

		acpi_pci_scanbus(subdev, subbus, &brirqs);
	}
	
}

static int
acpi_pci_scanbus(void *root, int bus, struct bridgeirqs *brirqs)
{
	uint64_t reg;
	ACPI_PCI_ID acpi_pciid;
	int dev, func, nfuncs;

	acpi_pciid.Segment = 0;
	acpi_pciid.Bus = bus;
	for (dev = 0; dev < 32; dev++) {
		acpi_pciid.Device = dev;
		nfuncs = getnfuncs(bus, dev);
		for (func = 0; func < nfuncs; func++) {
			acpi_pciid.Function = func;
			AcpiOsReadPciConfiguration(&acpi_pciid, PCI_VENDOR_REG, &reg, 32);
			if (PCI_VENDOR(reg) == 0xffff ||
			    PCI_VENDOR(reg) == 0) {
				continue;
			}

			acpi_pci_scandevice(root, bus, dev, func, brirqs);
		}
	}
	return 0;
}

static int
acpi_pci_scan(void)
{
	int i, bus;
	void *pciroot;
	char pciname[13];
	struct device *tmp, *d = NULL;
	uint64_t pnpid = squoze("PNP0A03");

	/*
	 * Search for PCI root.
	 */

	/* Search for PNP0A03 in devices */
	SLIST_FOREACH(tmp, &devices, list) {
		for (i = 0; i < DEVICEIDS; i++) {
			if (tmp->deviceids[i] == pnpid)  {
				d = tmp;
				break;
			}
		}
	}
	if (d == NULL) {
		printf("PCI root device not found\n");
		return -1;
	}

	unsquozelen(d->nameid, 13, pciname);
	printf("PCI root device found at %s\n", pciname);

	pciroot = platform_getdev(d->nameid);
	return acpi_pci_scanbus(pciroot, 0, NULL);
}


/*
 * Platform Exported Functions.
 */


struct prtrsctx {
	int idx;
	int irq;
};

static ACPI_STATUS
__prt_resource_irq(ACPI_RESOURCE *res, void *ctx)
{
	struct prtrsctx *prtrs = (struct prtrsctx *)ctx;
	struct sys_hwcreat_cfg *cfg = (struct sys_hwcreat_cfg *)ctx;

	switch (res->Type) {
	case ACPI_RESOURCE_TYPE_IRQ: {
		ACPI_RESOURCE_IRQ *p = &res->Data.Irq;

		if (!p || !p->InterruptCount || !p->Interrupts[0])
			return AE_OK;

		prtrs->irq = p->Interrupts[prtrs->idx];
		return -1;
	}
	case ACPI_RESOURCE_TYPE_EXTENDED_IRQ: {
		ACPI_RESOURCE_EXTENDED_IRQ *p = &res->Data.ExtendedIrq;

		if (!p || !p->InterruptCount || !p->Interrupts[0])
			return AE_OK;

		prtrs->irq = p->Interrupts[prtrs->idx];
		return -1;
	}
	default:
		prtrs->irq = 0;
		return AE_OK;
	}
}

int
platform_getpciintrs(void *dev, int devno, int ints[4])
{
	int i;
	void *lnkobj;
	struct prtrsctx ctx;
	char prtbuf[ACPI_BUFFER_SIZE];
	ACPI_BUFFER buffer;
	ACPI_STATUS status;
	ACPI_PCI_ROUTING_TABLE *prt;

	for (i = 0; i < 4; i++)
		ints[i] = 0;

	buffer.Pointer = prtbuf;
	buffer.Length = ACPI_BUFFER_SIZE;

	status = AcpiGetIrqRoutingTable(dev, &buffer);
	if (ACPI_FAILURE(status)) {
		printf("Could not acquire IRQ Routing table\n");
		return -1;
	}

	prt = (ACPI_PCI_ROUTING_TABLE *)buffer.Pointer;
	for (; prt->Length; prt = (ACPI_PCI_ROUTING_TABLE *)((uint8_t *)prt + prt->Length)) {
		int pdev = prt->Address >> 16;
		int irq = 0;

		if (pdev != devno)
			continue;

		if (prt->Source[0] == 0) {
			irq = prt->SourceIndex;
			goto __setirq;
		}

		status = AcpiGetHandle(dev, prt->Source, &lnkobj);
		if (ACPI_FAILURE(status)) {
			printf("Couldn't get IRQ source '%s' for pin %d\n", prt->Source, prt->Pin);
			continue;
		}

		ctx.irq = 0;
		ctx.idx = prt->SourceIndex;
		AcpiWalkResources(lnkobj, METHOD_NAME__CRS, __prt_resource_irq, &ctx);
		irq = ctx.irq;

	__setirq:
		if (prt->Pin < 4)
			ints[prt->Pin] = irq;
		else
			printf("Invalid pin %d in _PRT");
	}

	return 0;
}


int platform_getpcibusno(void *pciroot)
{
	uint64_t value;
	ACPI_STATUS status;

	status = AcpiUtEvaluateNumericObject(METHOD_NAME__BBN, pciroot, &value);
	if (ACPI_FAILURE(status))
		return -1;

	return (int)value;
}


int platform_getpciaddr(void *dev, int *devno, int *funcno)
{
	uint64_t value;
	ACPI_STATUS status;

	status = AcpiUtEvaluateNumericObject(METHOD_NAME__ADR, dev, &value);
	if (ACPI_FAILURE(status))
		return -1;

	*devno = ACPI_HIWORD(ACPI_LODWORD(value));
	*funcno = ACPI_LOWORD(ACPI_LODWORD(value));
	
	return 0;
}


uint64_t platform_getname(void *dev)
{
	char name[16];
	ACPI_BUFFER buffer;
	ACPI_STATUS status;

	memset(name, 0, 16);

	buffer.Pointer = name;
	buffer.Length = 16;
	status = AcpiGetName(dev, ACPI_SINGLE_NAME, &buffer);
	if (ACPI_FAILURE(status)) {
		printf("AcpiGetName failed: %s",
		       AcpiFormatException(status));
		return 0;
	}

	return squoze(name);
}


void *platform_iterate_subdev(void *dev, void *last)
{
	ACPI_STATUS status;
	void *next;

	status = AcpiGetNextObject(ACPI_TYPE_DEVICE, dev, last, &next);

	if (ACPI_FAILURE(status))
		return NULL;
	return next;
}


void *platform_getpcisubdev(void *pciroot, int devno, int funcno)
{
	void *last = NULL;
	int ldev = -1, lfunc = -1;

	while ((last = platform_iterate_subdev(pciroot, last)) != NULL) {
		platform_getpciaddr(last, &ldev, &lfunc);

		if ((ldev == devno) && (lfunc == funcno))
			break;
	}

	return last;
}


struct findnamectx {
	char *name;
	void *dev;
};

static ACPI_STATUS
__find_name(ACPI_HANDLE devhandle, UINT32 level, void *opq, void **retval)
{
	struct findnamectx *ctx = (struct findnamectx *)opq;
	char name[16];
	ACPI_BUFFER buffer;
	ACPI_STATUS status;

	buffer.Pointer = name;
	buffer.Length = 16;
	status = AcpiGetName(devhandle, ACPI_SINGLE_NAME, &buffer);
	if (ACPI_FAILURE(status)) {
		printf("AcpiGetName failed: %s",
		       AcpiFormatException(status));
		return status;
	}

	if (!memcmp(name, ctx->name, sizeof(ctx->name))) {
		ctx->dev = devhandle;
		return -1;
	}

	return AE_OK;
}


void *platform_getdev(uint64_t name)
{
	ACPI_STATUS status;
	struct findnamectx ctx;

	ctx.name = unsquoze(name);
	ctx.dev = NULL;

	AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT, INT_MAX,
			  __find_name, NULL, &ctx, NULL);

	free(ctx.name);
	return ctx.dev;
}


int
platform_init(void)
{
	int ret;
	ACPI_STATUS as;
	unsigned int ptr;
	char pciroot[13];


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

	printf("ACPI initialized and loaded. Enabling.\n");

	as = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if (ACPI_FAILURE(as))
		return -6;


	as = acpi_set_int(NULL, "_PIC", 1);
	if (ACPI_FAILURE(as) && (as != AE_NOT_FOUND))
		return -7;

	/*
	 * Scan ACPI devices.
	 */

	/*
	 * Get non-enumerable devices in system bus. They are
	 * recognizable by the _CRS field.
	 */

	printf("ACPI Platform devices:\n");
	AcpiWalkNamespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT, INT_MAX,
			  acpi_per_device, NULL, NULL, NULL);

	/*
	 * Find PCI root among all devices installed and scan the ACPI namespace.
	 */

	ret = acpi_pci_scan();
	if (ret)
		return ret;

	return 0;
}

