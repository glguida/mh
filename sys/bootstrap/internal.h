#ifndef _internal_h
#define _internal_h

#include <sys/null.h>
#include <inttypes.h>
#include <sys/queue.h>

struct sys_hwcreat_cfg;

extern SLIST_HEAD(device_list, device) devices;

#define DEVICEIDS 8

struct device {
	uint64_t nameid;
	uint64_t vendorid;
	uint64_t deviceids[8];
	SLIST_ENTRY(device) list;
};

/* platform devices */
int pltacpi_init(void);
int pltpci_init(void);

/* device management */
void devadd(struct sys_hwcreat_cfg *cfg);

int pltconsole_process(void);

#endif
