#include <sys/types.h>
#include <sys/errno.h>
#include <sys/queue.h>
#include <machine/vmparam.h>
#include <microkernel.h>
#include <sys/dirtio.h>
#include <assert.h>
#include "vmap.h"

#define MEMCACHESIZE 16L

static struct memcache {
	unsigned id;
	ioaddr_t ioaddr;
	void *vaddr;
	unsigned ref;
	TAILQ_ENTRY(memcache) list;
} memcache[MEMCACHESIZE];

static TAILQ_HEAD(mchead ,memcache) memcache_lruq =
	TAILQ_HEAD_INITIALIZER(memcache_lruq);

void *dirtio_mem_get(unsigned id, ioaddr_t ioaddr)
{
	int ret;
	struct memcache *mc;

	/* Search by the LRU */
	TAILQ_FOREACH_REVERSE(mc, &memcache_lruq, mchead, list) {
		if (mc->id == id && mc->ioaddr == ioaddr) {
			mc->ref++;
			return mc->vaddr;
		}
	}

	mc = TAILQ_FIRST(&memcache_lruq);
	TAILQ_REMOVE(&memcache_lruq, mc, list);
	assert(mc->ref == 0);

	ret = sys_import(id, ioaddr, (vaddr_t)mc->vaddr);
	if (ret != 0) {
		TAILQ_INSERT_HEAD(&memcache_lruq, mc, list);
		return NULL;
	}
	TAILQ_INSERT_TAIL(&memcache_lruq, mc, list);

	mc->ref++;
	mc->id = id;
	mc->ioaddr = ioaddr;
	return mc->vaddr;
}

void dirtio_mem_put(void *addr)
{
	struct memcache *mc;

	/* XXX: faster way: use addr - start to get memcache entry */
	TAILQ_FOREACH_REVERSE(mc, &memcache_lruq, mchead, list)
		if (mc->vaddr == addr) {
			assert(mc->ref != 0);
			mc->ref--;
			return;
		}
	assert(0);
}

void dirtio_mem_init(void)
{
	int i;
	vaddr_t start = vmap_alloc(MEMCACHESIZE * PAGE_SIZE, VFNT_IMPORT);

	for (i = 0; i < MEMCACHESIZE; i++) {
		memcache[i].id = -1;
		memcache[i].ioaddr = 0;
		memcache[i].vaddr = (void *)(start + (i * PAGE_SIZE));
		memcache[i].ref = 0;
		TAILQ_INSERT_HEAD(&memcache_lruq, memcache + i, list);
	}
}

int dirtio_dev_return(unsigned id, uint32_t val)
{
	struct dirtio_hdr *hdr;

	/* XXX: Do a usercpy-like function. We might fault if remote
	 * doesn't export memory! */
	hdr = (struct dirtio_hdr *)dirtio_mem_get(id, 0);
	if (hdr == NULL)
		return -ENOENT;
	hdr->ioval = val;
	dirtio_mem_put(hdr);
	return 0;
}

int dirtio_dev_io(struct dirtio_dev *dev,
		  unsigned id, uint64_t port, uint64_t val)
{
	int ret = -EINVAL;
	int read = 0;
	unsigned field, queue;

	if (port & PORT_DIRTIO_IN)
		read = 1;

	port &= ~PORT_DIRTIO_IN;
	field = port >> 8;
	queue = port & 0xff;
	if (read) {
		/* Read */
	  
		switch (field) {
		case PORT_DIRTIO_MAGIC:
			val = DIRTIO_MAGIC;
			break;
		case PORT_DIRTIO_VER:
			val = DIRTIO_VER;
			break;
		case PORT_DIRTIO_QMAX:
			if (queue >= dev->queues)
				goto read_err;
			val = dev->qmax[queue];
			break;
		case PORT_DIRTIO_QSIZE:
			if (queue >= dev->queues)
				goto read_err;
			val = dev->qsize[queue];
			break;
		case PORT_DIRTIO_READY:
			if (queue >= dev->queues)
				goto read_err;
			val = dev->qready[queue];
			break;
		case PORT_DIRTIO_ISR:
			val = dev->isr;
			break;
		case PORT_DIRTIO_DSR:
			val = dev->dsr;
			break;
		read_err:
		default:
			val = -1;
			break;
		}
		ret = dirtio_dev_return(id, val);
	} else {
		/* Write */
		field = (port) >> 8;
		queue = (port) & 0xff;

		switch (field) {
		case PORT_DIRTIO_QSIZE:
			if (queue < dev->queues && val < dev->qmax[queue]) {
				dev->qsize[queue] = val;
				ret = 0;
			}
			break;
		case PORT_DIRTIO_NOTIFY:
			if (queue < dev->queues)
				ret = dev->notify(queue);
			break;
		default:
			break;
		}
	}
	return ret;
    
}

void
dirtio_dev_init(struct dirtio_dev *dev, unsigned queues,
		unsigned *qmax, unsigned *qsize, unsigned *qready)
{
	dirtio_mem_init();
	dev->queues = queues;
	dev->qmax = qmax;
	dev->qsize = qsize;
	dev->qready = qready;
	dev->isr = 0;
	dev->dsr = 0;
}

