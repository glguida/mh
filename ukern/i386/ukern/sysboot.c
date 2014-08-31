#include <uk/types.h>
#include <uk/param.h>
#include <lib/lib.h>

#include <ukern/pfndb.h>

char *_boot_cmdline = NULL;

struct e820e {
	uint64_t addr;
	uint64_t len;
	uint32_t type;
	uint32_t acpi;
} __packed *p;

void
sysboot(void)
{
    uint64_t i;

    if (_boot_cmdline != NULL)
    	printf("\ncmdline: %s\n\n", _boot_cmdline);

    //    _setpfndb((void *)UKERN_PFNDB_PHYSADDR);
    _setpfndb((void *)(unsigned long)(16*1024*1024));

    printf("E820 map:\n");
    p = (struct e820e *)UKERN_BSMAP16;
    while (p->addr || p->len || p->type) {
	int pfnt = p->type == 1 ? PFN_FREE : PFN_IOMAP;

	printf("\t%016llx:%016llx -> %10s(%ld)\n", 
	       p->addr, p->len,
	       p->type == 1 ? "Memory" : "Reserved",
	       (long)p->type);


	for (i = p->addr; i < p->addr + p->len; i += 4096 /* PAGESIZE */)
	    pfndb_inittype((int)(i >> 12) /* PAGE_SHIFT */, pfnt);
	p++;
    }
    printf("\n");

    pfndb_printstats();
}

