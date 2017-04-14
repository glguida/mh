#include <mrg.h>
#include <microkernel.h>
#include <squoze.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/errno.h>
#include <machine/vmparam.h>

#define perror(...) printf(__VA_ARGS__)
#warning add perror

struct _DEVICE {
	int dd;

	struct dinfo info;

	unsigned ndevids;
	uint64_t *devids;

	int nirqsegs;
	struct sys_rdcfg_seg *irqsegs;

	int niosegs;
	struct sys_rdcfg_seg *iosegs;

	int nmemsegs;
	struct sys_rdcfg_memseg *memsegs;

	uint64_t usercfg[SYS_RDCFG_MAXUSERCFG];
};

static void __irq_handler(int irqint, void *arg)
{
	int irqevt = (int) (uintptr_t) arg;

	__evtset(irqevt);
}

DEVICE *dopen(char *devname)
{
	DEVICE *d;
	int i, ret, dd;
	uint64_t nameid;
	struct sys_rdcfg_cfg cfg;

	d = malloc(sizeof(*d));
	if (d == NULL) {
		perror("malloc");
		return NULL;
	}
	memset(d, 0, sizeof(*d));

	nameid = squoze(devname);
	dd = sys_open(nameid);
	if (dd < 0) {
		perror("sys_open");
		free(d);
		return NULL;
	}
	d->dd = dd;

	ret = sys_rdcfg(dd, &cfg);
	if (ret < 0) {
		perror("sys_rdcfg");
		sys_close(dd);
		free(d);
		return NULL;
	}

	assert(nameid == cfg.nameid);

	assert(sizeof(d->usercfg) == sizeof(cfg.usercfg));
	memcpy(d->usercfg, cfg.usercfg, sizeof(cfg.usercfg));


	/*
	 * Fill info
	 */

	d->info.nameid = cfg.nameid;
	d->info.vendorid = cfg.vendorid;
	/* count device ids */
	for (i = 0; i < SYS_RDCFG_MAX_DEVIDS; i++)
		if (cfg.deviceids[i] == 0)
			break;
	d->info.ndevids = i;
	/* count irqs */
	d->info.nirqs = 0;
	if (cfg.nirqsegs != (uint8_t) - 1)
		for (i = 0; i < cfg.nirqsegs; i++)
			d->info.nirqs += SYS_RDCFG_IRQSEG(&cfg, i).len;
	/* count pios */
	d->info.npios = 0;
	if (cfg.npiosegs != (uint8_t) - 1)
		for (i = 0; i < cfg.npiosegs; i++)
			d->info.npios += SYS_RDCFG_IOSEG(&cfg, i).len;
	d->info.nmemsegs = cfg.nmemsegs;


	/*
	 * devids
	 */
	d->ndevids = d->info.ndevids;
	if (d->ndevids == -1)
		goto skip_devids;
	d->devids = malloc(sizeof(uint64_t) * d->info.ndevids);
	if (d->devids == NULL) {
		perror("malloc");
		free(d);
		return NULL;
	}
	for (i = 0; i < d->ndevids; i++)
		d->devids[i] = cfg.deviceids[i];
      skip_devids:

	/*
	 * irqsegs
	 */
	d->nirqsegs = (cfg.nirqsegs == (uint8_t) - 1 ? -1 : cfg.nirqsegs);
	if (d->nirqsegs == -1)
		goto skip_irqsegs;
	d->irqsegs = malloc(sizeof(struct sys_rdcfg_seg) * d->nirqsegs);
	if (d->irqsegs == NULL) {
		perror("malloc");
		free(d->devids);
		free(d);
		return NULL;
	}
	for (i = 0; i < d->nirqsegs; i++)
		d->irqsegs[i] = SYS_RDCFG_IRQSEG(&cfg, i);
      skip_irqsegs:

	/*
	 * iosegs
	 */
	d->niosegs = (cfg.npiosegs == (uint8_t) - 1 ? -1 : cfg.npiosegs);
	if (d->niosegs == -1)
		goto skip_iosegs;
	d->iosegs = malloc(sizeof(struct sys_rdcfg_seg) * d->niosegs);
	if (d->iosegs == NULL) {
		perror("malloc");
		free(d->irqsegs);
		free(d->devids);
		free(d);
		return NULL;
	}
	for (i = 0; i < d->niosegs; i++)
		d->iosegs[i] = SYS_RDCFG_IOSEG(&cfg, i);
      skip_iosegs:

	/*
	 * memsegs
	 */
	d->nmemsegs = (cfg.nmemsegs == (uint8_t) - 1 ? -1 : cfg.nmemsegs);
	if (d->nmemsegs == -1)
		goto skip_memsegs;
	d->memsegs =
		malloc(sizeof(struct sys_rdcfg_memseg) * d->info.nmemsegs);
	if (d->memsegs == NULL) {
		perror("malloc");
		free(d->iosegs);
		free(d->irqsegs);
		free(d->devids);
		free(d);
		return NULL;
	}
	for (i = 0; i < d->nmemsegs; i++)
		d->memsegs[i] = cfg.memsegs[i];
      skip_memsegs:

	return d;
}

int din(DEVICE * d, uint32_t port, uint64_t * val)
{

	return sys_in(d->dd, port, val);
}


int dout(DEVICE * d, uint32_t port, uint64_t val)
{

	return sys_out(d->dd, port, val);
}

int dmapirq(DEVICE * d, unsigned irq, int evt)
{
	int ret, irqint;

	irqint = intalloc();
	ret = sys_mapirq(d->dd, irq, irqint);
	if (ret != 0) {
		intfree(irqint);
		return ret;
	}

	inthandler(irqint, __irq_handler, (void *) (uintptr_t) evt);
	return 0;
}

int dgetirq(DEVICE * d, int irqno)
{
	int i, rem;

	rem = irqno;
	for (i = 0; i < d->nirqsegs, rem >= 0; i++) {
		uint16_t base = d->irqsegs[i].base;
		uint16_t len = d->irqsegs[i].len;

		if (rem < len)
			return base + rem;
		else
			rem -= len;
	}
	return -ENOENT;
}

int dgetpio(DEVICE * d, int piono)
{
	int i, rem;

	rem = piono;
	for (i = 0; i < d->niosegs, rem >= 0; i++) {
		uint16_t base = d->iosegs[i].base;
		uint16_t len = d->iosegs[i].len;

		if (rem < len)
			return base + rem;
		else
			rem -= len;
	}
	return -ENOENT;
}

ssize_t dgetmemrange(DEVICE * d, unsigned rangeno, uint64_t * base)
{
	int i, rem;
	struct sys_rdcfg_memseg *seg;

	if (rangeno >= d->nmemsegs)
		return -ENOENT;

	seg = d->memsegs + rangeno;

	if (base)
		*base = seg->base;
	return seg->len;
}

uint64_t dusercfg(DEVICE * d, unsigned i)
{
	if (i >= SYS_RDCFG_MAXUSERCFG)
		return 0;

	return d->usercfg[i];
}

int dgetinfo(DEVICE * d, struct dinfo *i)
{

	*i = d->info;
	return 0;
}

void *diomap(DEVICE * d, uint64_t base, size_t len)
{
	vaddr_t va;
	unsigned i, pages, ret;

	pages = round_page(len) >> PAGE_SHIFT;
	va = vmap_alloc(len, VFNT_MMIO);
	if (va == 0)
		return NULL;

	for (i = 0; i < pages; i++) {
		ret = sys_iomap(d->dd,
				va + (i << PAGE_SHIFT),
				trunc_page(base) + (i << PAGE_SHIFT));
	}
	if (ret < 0) {
		int j;
		for (j = 0; j < i; j++)
			sys_iounmap(d->dd, va + (j << PAGE_SHIFT));
		return NULL;
	}

	return (void *) (uintptr_t) (va + (base & PAGE_MASK));
}

int dexport(DEVICE * d, void *vaddr, unsigned long *iopfn)
{
	return sys_export(d->dd, (vaddr_t) vaddr, iopfn);
}

void dclose(DEVICE * d)
{
	sys_close(d->dd);
	free(d->devids);
	free(d->irqsegs);
	free(d->iosegs);
	free(d->memsegs);
	free(d);
}
