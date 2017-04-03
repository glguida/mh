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
int platform_init(void);
void *platform_getdev(uint64_t name);
uint64_t platform_getname(void *dev);
void *platform_iterate_subdev(void *dev, void *last);

int platform_getpcibusno(void *pciroot);
void *platform_getpcisubdev(void *pciroot, int devno, int funcno);
int platform_getpciaddr(void *dev, int *devno, int *funcno);
int platform_getpciintrs(void *pciroot, int dev, int ints[4]);

int pltpci_init(void);

/* device management */
void devadd(struct sys_hwcreat_cfg *cfg);

int pltusb_process(void);
int pltconsole_process(void);

void kbdbuf_add(int id, uint8_t c);
int console_kbd_init(uint64_t nameid);
int console_vga_init(uint64_t nameid);

#endif
