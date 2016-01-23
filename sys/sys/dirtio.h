#ifndef _sys_dirtio_h
#define _sys_dirtio_h

#include <microkernel.h>

#define DIRTIO_VID		0x00f00ffa

#define PORT_DIRTIO_IN		0x10000

#define DIRTIO_MAGIC 		0x74726976
#define DIRTIO_VER		0x00000000

#define PORT_DIRTIO_MAGIC	0x1
#define PORT_DIRTIO_VER		0x2
#define PORT_DIRTIO_ISR		0x3
#define PORT_DIRTIO_DSR		0x4

/* Per-queue */
#define PORT_DIRTIO_QMAX	0x10
#define PORT_DIRTIO_QSIZE	0x11
#define PORT_DIRTIO_READY 	0x12
#define PORT_DIRTIO_NOTIFY	0x13



#include <sys/dirtio_types.h>
#include <sys/dirtio_config.h>
#include <sys/dirtio_ring.h>

struct dirtio_hdr {
  __dirtio32 ioval;
  struct dring ring;

};

struct dirtio_dev {
	unsigned queues;
	unsigned *qmax;
	unsigned *qsize;
	unsigned *qready;
	uint32_t isr; /* Interrupt Status Register */
	uint32_t dsr; /* Device Status Register */

	int (*notify)(unsigned queue);
};

struct dirtio_desc {
	int id;
	int eiosig;
	struct dirtio_hdr *hdr;

	uint64_t nameid;
	uint64_t vendorid;
	uint64_t deviceid;
	unsigned eioirq;
};

void dirtio_dev_init(unsigned queues,
		     unsigned *qmax, unsigned *qsize, unsigned *qready);
int dirtio_dev_creat(struct sys_creat_cfg *cfg, devmode_t mode);


int dirtio_open(struct dirtio_desc *dio, uint64_t nameid,
		struct dirtio_hdr *hdr);
int dirtio_mmio_inw(struct dirtio_desc *dio, uint32_t port, uint8_t queue,
		    uint64_t *val);
int dirtio_mmio_out(struct dirtio_desc *dio, uint32_t port, uint8_t queue,
		    uint64_t val);
int dirtio_mmio_outw(struct dirtio_desc *dio, uint32_t port, uint8_t queue,
		     uint64_t val);
#endif
