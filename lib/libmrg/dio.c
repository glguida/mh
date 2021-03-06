#include <mrg.h>
#include <microkernel.h>
#include <squoze.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/errno.h>
#include <machine/vmparam.h>

struct _DEVICE {
	int dd;
	uint64_t nameid;
	uint64_t vendorid;

	unsigned ndevids;
	uint64_t *devids;

	unsigned nirqs;
	unsigned  nirqsegs;
	struct sys_info_seg *irqsegs;

	unsigned nios;
	unsigned niosegs;
	struct sys_info_seg *iosegs;

	unsigned nmemsegs;
	struct sys_info_memseg *memsegs;
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
	struct sys_info_cfg cfg;

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

	ret = sys_info(dd, &cfg);
	if (ret < 0) {
		perror("sys_info");
		sys_close(dd);
		free(d);
		return NULL;
	}

	assert(nameid == cfg.nameid);
	d->nameid = cfg.nameid;
	d->vendorid = cfg.vendorid;


	/*
	 * devids
	 */
	for (i = 0; i < SYS_INFO_MAX_DEVIDS; i++)
		if (cfg.deviceids[i] == 0)
			break;
	d->ndevids = i;
	if (d->ndevids == 0)
		goto skip_devids;
	d->devids = malloc(sizeof(uint64_t) * d->ndevids);
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
	d->nirqs = 0;
	if (cfg.nirqsegs != (uint8_t) - 1)
		for (i = 0; i < cfg.nirqsegs; i++)
			d->nirqs += SYS_INFO_IRQSEG(&cfg, i).len;
	d->nirqsegs = (cfg.nirqsegs == (uint8_t) - 1 ? 0 : cfg.nirqsegs);
	if (d->nirqsegs == 0)
		goto skip_irqsegs;
	d->irqsegs = malloc(sizeof(struct sys_info_seg) * d->nirqsegs);
	if (d->irqsegs == NULL) {
		perror("malloc");
		free(d->devids);
		free(d);
		return NULL;
	}
	for (i = 0; i < d->nirqsegs; i++)
		d->irqsegs[i] = SYS_INFO_IRQSEG(&cfg, i);
      skip_irqsegs:

	/*
	 * iosegs
	 */
	d->nios = 0;
	if (cfg.npiosegs != (uint8_t) - 1)
		for (i = 0; i < cfg.npiosegs; i++)
			d->nios += SYS_INFO_IOSEG(&cfg, i).len;
	d->niosegs = (cfg.npiosegs == (uint8_t) - 1 ? 0 : cfg.npiosegs);
	if (d->niosegs == 0)
		goto skip_iosegs;
	d->iosegs = malloc(sizeof(struct sys_info_seg) * d->niosegs);
	if (d->iosegs == NULL) {
		perror("malloc");
		free(d->irqsegs);
		free(d->devids);
		free(d);
		return NULL;
	}
	for (i = 0; i < d->niosegs; i++)
		d->iosegs[i] = SYS_INFO_IOSEG(&cfg, i);
      skip_iosegs:

	/*
	 * memsegs
	 */
	d->nmemsegs = (cfg.nmemsegs == (uint8_t) - 1 ? 0 : cfg.nmemsegs);
	if (d->nmemsegs == 0)
		goto skip_memsegs;
	d->memsegs =
		malloc(sizeof(struct sys_info_memseg) * d->nmemsegs);
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

int deoi(DEVICE * d, unsigned irq)
{
	return sys_eoi(d->dd, irq);
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
	struct sys_info_memseg *seg;

	if (rangeno >= d->nmemsegs)
		return -ENOENT;

	seg = d->memsegs + rangeno;

	if (base)
		*base = seg->base;
	return seg->len;
}

int drdcfg(DEVICE * d, unsigned off, uint8_t sz, uint64_t *val)
{
	return sys_rdcfg(d->dd, off, sz, val);
}

int dwrcfg(DEVICE *d, unsigned off, uint8_t sz, uint64_t val)
{
	return sys_wrcfg(d->dd, off, sz, &val);
}

int dgetinfo(DEVICE * d, struct dinfo *i)
{
	i->nameid = d->nameid;
	i->vendorid = d->vendorid;

	i->ndevids = d->ndevids;
	i->nirqs = d->nirqs;
	i->npios = d->nios;
	i->nmemsegs = d->nmemsegs;

	i->devids = d->ndevids > 0 ? d->devids : NULL;
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

int dexport(DEVICE * d, void *vaddr, size_t sz, iova_t *iova)
{
	return sys_export(d->dd, (vaddr_t) vaddr, sz, iova);
}

int dunexport(DEVICE * d, void *vaddr)
{
	return sys_unexport(d->dd, (vaddr_t) vaddr);
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
