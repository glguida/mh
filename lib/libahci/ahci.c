#include <sys/types.h>
#include <sys/bitops.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <machine/vmparam.h>
#include <squoze.h>
#include <mrg.h>
#include <mrg/blk.h>
#include "ahci_hw.h"
#include "ahci.h"

#ifndef __DEBUG
#define dbgprintf(...)
#else
#define dbgprintf(...) printf(__VA_ARGS__)
#endif

enum blk_op {
	BLK_OP_READ,
	BLK_OP_WRITE,
};

struct blk_ahci_hw_hba;
struct blk_ahci_hw_port;

struct blk_ahci_qop {
	enum blk_op op;
	void *data;
	size_t sz;
	uint64_t blkid;
	size_t nblks;
	int evt;
	int *retp;

	TAILQ_ENTRY(blk_ahci_qop) ioqe;
};

struct blk_ahci_port {
	uint32_t lastci;
	void *cmd_addr[32];
	size_t cmd_sz[32];
	int *cmd_retp[32];
	int cmd_evt[32];
};

struct blk_ahci {
	uint64_t devid;
	DEVICE *d;
	uint64_t basename;

	int irq;
	int evt;

	void *mem;
	iova_t mem_iova;

	uint32_t ports_impl;
	TAILQ_HEAD(, blk_ahci_qop) ioq;
	struct blk_ahci_port ports[32];

	volatile struct blk_ahci_hw_hba *hw_hba;
	volatile struct blk_ahci_hw_port *hw_ports;
};

struct blk_ahci_opq {
	struct blk_ahci *ahci;
	int port;
};


#define blk_ahci_clb(_a, _i) ((_a)->mem + ((_i) << 10))
#define blk_ahci_clb_iova(_a, _i) ((uint64_t)((_a)->mem_iova + ((_i) << 10)))
#define blk_ahci_fb(_a, _i) ((_a)->mem + (32 << 10) + ((_i) << 8))
#define blk_ahci_fb_iova(_a, _i) ((uint64_t)((_a)->mem_iova + (32 << 10) + ((_i) << 8)))
#define blk_ahci_ctba(_a, _i, _j) ((_a)->mem + (40 << 10) + ((_i) << 13) + ((_j) << 8))
#define blk_ahci_ctba_iova(_a, _i, _j) ((uint64_t)(_a)->mem_iova + (40 << 10) + ((_i) << 13) + ((_j) << 8))
#define blk_ahci_memsize() ((40 << 10) + (33 << 13))


#define blk_ahci_ch(_a, _i, _j) ((volatile struct blk_ahci_hw_cmdhdr *)(blk_ahci_clb((_a), (_i))) + (_j))
#define blk_ahci_cfis(_a, _i, _j) ((volatile void *)blk_ahci_ctba((_a), (_i), (_j)))
#define blk_ahci_prdtl(_a, _i, _j) ((volatile struct blk_ahci_hw_prdt *)(blk_ahci_ctba((_a), (_i), (_j)) + 0x80))

#define AHCI_PORT_PRDT_SIZE 8
/* For simplicity (and probable lack of consecutive pages) we
 * use a page per PRDT entry.
 *
 * If va is not aligned, we would could fit no more than (n-1)
 * full pages in the PRDT, where n is the size of the PDT.
 * Reflect that by only allowing (n - 1) maximum pages.
 */
#define AHCI_PORT_DMA_MAXSIZE ((AHCI_PORT_PRDT_SIZE - 1) * PAGE_SIZE)


#define AHCI_LOG(_a, ...) do { dbgprintf("AHCI %s: ", unsquoze_inline((_a)->devid).str); dbgprintf(__VA_ARGS__); dbgprintf("\n"); } while(0)
#define AHCI_PORT_LOG(_a, _i, ...) do { dbgprintf("AHCI %s port %d: ", unsquoze_inline((_a)->devid).str, i); dbgprintf(__VA_ARGS__); dbgprintf("\n"); } while(0)
#define AHCI_PORT_INFO(_a_, _i, ...) do { printf("AHCI %s port %d: ", unsquoze_inline((_a)->devid).str, i); printf(__VA_ARGS__); printf("\n"); } while(0)
#define AHCI_PORT_ERR(_a, _i, ...) do { dbgprintf("AHCI %s port %d ERROR: ", unsquoze_inline((_a)->devid).str, i); dbgprintf(__VA_ARGS__); dbgprintf("\n"); } while(0)

static void ahci_port_unset_prdt(struct blk_ahci *ahci, int i, int j, void *addr, size_t sz);

static void ahci_port_intr(struct blk_ahci *ahci, int i)
{
	int j;
	volatile struct blk_ahci_hw_port *hwp= ahci->hw_ports + i;
	struct blk_ahci_port *p = ahci->ports + i;
	uint32_t ci;
	uint32_t done;

	hwp->is = -1;
	__compiler_membar();

	ci = hwp->ci;
	done = p->lastci & ~ci;
	while (done) {
		for (j = 0; j < 32; j++) {
			if (done & (1 << j)) {
				AHCI_PORT_LOG(ahci, i, "CMD %d DONE", j);
				ahci_port_unset_prdt(ahci, i, j, p->cmd_addr[j], p->cmd_sz[j]);
				evtset(p->cmd_evt[j]);
				*p->cmd_retp[j] = 0;
			}
		}
		p->lastci = ci;
		ci = hwp->ci;
		done = p->lastci & ~ci;
		
	}
}

static int ahci_port_error(struct blk_ahci *ahci, unsigned i)
{
	int err;
	volatile struct blk_ahci_hw_port *p = ahci->hw_ports + i;
	unsigned err_intrs = PX_IS_IFS | PX_IS_HBDS | PX_IS_HBFS | PX_IS_TFES;

	err = !!(p->is & err_intrs);

	if (err) {
		AHCI_PORT_ERR(ahci, i, "I/O ERR PxIS: %08lx PxTFD: %08lx PxSERR: %08lx",
			      p->is, p->tfd, p->serr);
	}

	return err;
}

static int ahci_port_get_slot(struct blk_ahci *ahci, unsigned i)
{
	int j;
	uint32_t free;

	free = ~ahci->ports[i].lastci & ~ahci->hw_ports[i].sact;
	j = ffs32((long)free);
	if (j == 0)
		return -EBUSY;

	return j - 1;
}

static int ahci_port_set_prdt(struct blk_ahci *ahci, int i, int j, volatile struct blk_ahci_hw_cmdhdr *ch, void *addr, size_t sz)
{
	int ret;
	iova_t iova;
	size_t chunk;
	volatile struct blk_ahci_hw_prdt *prdt;
	unsigned k, n = (round_page((vaddr_t)addr + sz) - trunc_page((vaddr_t)addr)) >> PAGE_SHIFT;

	if (sz >= AHCI_PORT_DMA_MAXSIZE)
		return -ENOMEM;

	for (k = 0; k < n; k++) {
		chunk = PAGE_SIZE - ((vaddr_t)addr & PAGE_MASK);
		chunk = sz > chunk ? chunk : sz;
		ret = dexport(ahci->d, addr, chunk, &iova);
		if (ret) {
			unsigned l;
			for (l = 0; l < k; l++)
				dunexport(ahci->d, addr + (l << PAGE_SHIFT));
			return ret;
		}

		prdt = blk_ahci_prdtl(ahci, i, j) + k;
		prdt->dba = (uint32_t)iova;
		prdt->dbau = iova >> 32;
		prdt->dbc = (chunk - 1);
		addr += chunk;
	}
	prdt->dbc |= PRDT_DBC_I;

	ch->opts |= bitfld_set(ch->opts, CMDHDR_PRDTL, n);
	return 0;
}

static void ahci_port_unset_prdt(struct blk_ahci *ahci, int i, int j, void *addr, size_t sz)
{
	int ret;
	iova_t iova;
	size_t chunk;
	volatile struct blk_ahci_hw_prdt *prdt;
	unsigned k, n = (round_page((vaddr_t)addr + sz) - trunc_page((vaddr_t)addr)) >> PAGE_SHIFT;

	for (k = 0; k < n; k++) {
		chunk = PAGE_SIZE - ((vaddr_t)addr & PAGE_MASK);
		chunk = sz > chunk ? chunk : sz;
		(void)dunexport(ahci->d, addr);
	}
}

static int ahci_port_set_fis(struct blk_ahci *ahci, int i, int j, volatile struct blk_ahci_hw_cmdhdr *ch, enum blk_op op,  uint64_t blkid, size_t nblks)
{
	volatile struct fis_reg_h2d *cfis;

	cfis = (volatile struct fis_reg_h2d *) blk_ahci_cfis(ahci, i, j);
	memset((void *)cfis, 0, sizeof(*cfis));
	cfis->fis = FIS_TYPE_REG_H2D;
	cfis->flags = FIS_FLAGS_C;
	cfis->dev = (1 << 6);

	switch(op) {
	case BLK_OP_READ:
		cfis->cmd = 0x25;
		break;
	case BLK_OP_WRITE:
		cfis->cmd = 0x35;
		ch->opts |= CMDHDR_W;
		break;
	default:
		return -ENODEV;
	};

	cfis->lba0 = blkid & 0xff;
	blkid >>= 8;
	cfis->lba1 = blkid & 0xff;
	blkid >>= 8;
	cfis->lba2 = blkid & 0xff;
	blkid >>= 8;
	cfis->lba3 = blkid & 0xff;
	blkid >>= 8;
	cfis->lba4 = blkid & 0xff;
	blkid >>= 8;
	cfis->lba5 = blkid & 0xff;

	cfis->count1 = nblks & 0xff;
	nblks >>= 8;
	cfis->count2 = nblks & 0xff;

	ch->opts |= bitfld_set(ch->opts, CMDHDR_CFL, 5);

	return 0;
}

static int ahci_port_issue_qop(struct blk_ahci *ahci, int i, int j, struct blk_ahci_qop *qop)
{
	int ret;
	volatile struct blk_ahci_hw_cmdhdr *ch;

	ch = blk_ahci_ch(ahci, i, j);
	memset((void *)ch, 0, sizeof(*ch));
	ch->ctba = (uint32_t) blk_ahci_ctba_iova(ahci, i, j);
	ch->ctbau = (uint32_t) (blk_ahci_ctba_iova(ahci, i, j) >> 32);

	ret = ahci_port_set_prdt(ahci, i, j, ch, qop->data, qop->sz);
	if (ret)
		return ret;

	ret = ahci_port_set_fis(ahci, i, j, ch, qop->op, qop->blkid, qop->nblks);
	if (ret)
		return ret;

#if 0
	int v;
	for (v = 0; v < 8; v++) {
		printf("CH[%d]: %llx\n", v, *((uint32_t *)blk_ahci_ch(ahci, i, j) + v));
	}

	for (v = 0; v < 5; v++) {
		printf("CFIS[%d]: %lx\n", v, *((uint32_t *)blk_ahci_cfis(ahci, i, j) + v));
	}

	for (v = 0; v < 4; v++) {
		printf("PRDT[%d]: %lx\n", v,
		       *((uint32_t *)blk_ahci_prdtl(ahci, i, j) + v));
	}
#endif

	ahci->ports[i].lastci |= (1 << j);
	ahci->ports[i].cmd_retp[j] = qop->retp;
	ahci->ports[i].cmd_evt[j] = qop->evt;
	ahci->ports[i].cmd_addr[j] = qop->data;
	ahci->ports[i].cmd_sz[j] = qop->sz;
	ahci->hw_ports[i].ci |= (1 << j);
	return 0;
}

static int ahci_port_queue_op(struct blk_ahci *ahci, int i, enum blk_op op, void *data, size_t sz, uint64_t blkid, size_t nblks, int evt, int *ret)
{
	struct blk_ahci_qop qop, *ptr;

	qop.op = op;
	qop.data = data;
	qop.sz = sz;
	qop.blkid = blkid;
	qop.nblks = nblks;
	qop.evt = evt;
	qop.retp = ret;

	if (TAILQ_EMPTY(&ahci->ioq)) {
		int j;

		j = ahci_port_get_slot(ahci, i);
		if (j >= 0)
			return ahci_port_issue_qop(ahci, i, j, &qop);
	}

	ptr = (struct blk_ahci_qop *)malloc(sizeof(*ptr));
	if (ptr == NULL)
		return -ENOMEM;

	*ptr = qop;
	TAILQ_INSERT_TAIL(&ahci->ioq, ptr, ioqe);
	return 0;
}

static int ahci_port_blkrd(void *opq, void *data, size_t sz, uint64_t blkid, size_t nblks, int evt, int *res)
{
	struct blk_ahci_opq *aopq = (struct blk_ahci_opq *)opq;

	return ahci_port_queue_op(aopq->ahci, aopq->port, BLK_OP_READ, data, sz, blkid, nblks, evt, res);
}

static int ahci_port_blkwr(void *opq, uint64_t blkid, size_t nblks, void *data, size_t sz, int evt, int *res)
{
	struct blk_ahci_opq *aopq = (struct blk_ahci_opq *)opq;

	return ahci_port_queue_op(aopq->ahci, aopq->port, BLK_OP_WRITE, data, sz, blkid, nblks, evt, res);
}

static const struct blkops ahci_blkops =  {
	.blkrd = ahci_port_blkrd,
	.blkwr = ahci_port_blkwr,
};

static void ahci_port_ata_get_blkinfo(struct blk_ahci *ahci, unsigned i, uint16_t *ident, struct blkinfo *bi)
{
	int l;
	uint16_t w;
	uint64_t sectsz;
	uint64_t sectno;
	char string[41];

	for (l = 0; l < 20; l++) {
		w = *(ident + ATA_IDENT_MODEL + l);
		string[2*l] = w >> 8;
		string[2*l + 1] = (uint8_t)w;
	}
	string[40] = '\0';
	AHCI_PORT_LOG(ahci, i, "ATA Device Model No: %s", string);

	for (l = 0; l < 4; l++) {
		w = *(ident + ATA_IDENT_FIRMWARE + l);
		string[2*l] = w >> 8;
		string[2*l + 1] = (uint8_t)w;
	}
	string[8] = '\0';
	AHCI_PORT_LOG(ahci, i, "ATA Device Firmware version: %s", string);

	for (l = 0; l < 5; l++) {
		w = *(ident + ATA_IDENT_SERIAL + l);
		string[2*l] = w >> 8;
		string[2*l + 1] = (uint8_t)w;
	}
	string[10] = '\0';
	AHCI_PORT_LOG(ahci, i, "ATA Device Serial No.: %s", string);

	sectno = *(uint64_t *)(ident + ATA_IDENT_SECTNO48);

	sectsz = 512;
	if ((ident[ATA_IDENT_SECTSZ] & ATA_IDENT_SECTSZ_CUSTOM_MASK) == ATA_IDENT_SECTSZ_CUSTOM) {
		sectsz = 2 * *(uint32_t *)(ident + ATA_IDENT_LOGSECTSZ);
	}
	
	AHCI_PORT_LOG(ahci, i, "ATA Device: %"PRId64" sectors", sectno);

	bi->blksz = sectsz;
	bi->blkno = sectno;
}

static int ahci_port_get_blkinfo(struct blk_ahci *ahci, unsigned i, struct blkinfo *bi)
{
	int j, ret;
	char ident[512];
	unsigned long n = (round_page((vaddr_t)ident + 512) - trunc_page((vaddr_t)ident)) >> PAGE_SHIFT;
	volatile struct blk_ahci_hw_cmdhdr *ch;
	volatile struct fis_reg_h2d *cfis;

	j = ahci_port_get_slot(ahci, i);

	ch = blk_ahci_ch(ahci, i, j);
	memset((void *)ch, 0, sizeof(*ch));

	ch->ctba = (uint32_t) blk_ahci_ctba_iova(ahci, i, j);
	ch->ctbau = (uint32_t) (blk_ahci_ctba_iova(ahci, i, j) >> 32);

	cfis = (volatile struct fis_reg_h2d *) blk_ahci_cfis(ahci, i, j);
	memset((void *)cfis, 0, sizeof(*cfis));
	cfis->fis = FIS_TYPE_REG_H2D;
	cfis->flags = FIS_FLAGS_C;
	cfis->cmd = 0xec;
	cfis->feature1 = 1;
	ch->opts = bitfld_set(ch->opts, CMDHDR_CFL, 5);

	ret = ahci_port_set_prdt(ahci, i, j, ch, ident, 512);
	if (ret)
		return ret;

	ahci->hw_ports[i].ci |= (1 << j);

	evtwait(ahci->evt);
	evtclear(ahci->evt);

	ahci_port_unset_prdt(ahci, i, j, ident, 512);

	if (ahci_port_error(ahci, i)) {
		/* No point in recovering. Not using this port. */
		return -EIO;
	}
	ahci->hw_ports[i].is = -1;
	assert(deoi(ahci->d, ahci->irq) == 0);

	ahci_port_ata_get_blkinfo(ahci, i, (uint16_t *)ident, bi);
	return 0;
}

static void ahci_port_init(struct blk_ahci *ahci, unsigned i)
{
	int ret;
	uint64_t name;
	struct blkinfo blkinfo;
	volatile struct blk_ahci_hw_cmdhdr *ch;
	volatile struct blk_ahci_hw_port *p = ahci->hw_ports + i;

	ahci->ports[i].lastci = 0;

	p->cmd &= ~PX_CMD_FRE;
	while (p->cmd & PX_CMD_FR) lwt_pause();

	p->cmd &= ~PX_CMD_ST;
	while (p->cmd & PX_CMD_CR) lwt_pause();

	/* Command List */
	memset(blk_ahci_clb(ahci, i), 0, 1024);
	p->clb = (uint32_t) blk_ahci_clb_iova(ahci, i);
	p->clbu = (uint32_t) (blk_ahci_clb_iova(ahci, i) >> 32);

	memset(blk_ahci_fb(ahci, i), 0, 256);
	p->fb = (uint32_t) blk_ahci_fb_iova(ahci, i);
	p->fbu = (uint32_t) (blk_ahci_fb_iova(ahci, i) >> 32);

	p->cmd |= PX_CMD_FRE;
	p->cmd |= PX_CMD_ST;

       	p->ie = PORT_INT_DHR | PORT_INT_PS | PORT_INT_DS
       		| PORT_INT_SDB | PORT_INT_UF | PORT_INT_DP;

	AHCI_PORT_LOG(ahci, i, "RAM structures initialised: CMDL %p [%lx] FB %p [%lx] CFIS: %p [%lx] PRDT %p",
	       blk_ahci_clb(ahci, i), (unsigned long) blk_ahci_clb_iova(ahci, i),
	       blk_ahci_fb(ahci, i), (unsigned long) blk_ahci_fb_iova(ahci, i),
	       blk_ahci_cfis(ahci, i, 0), (unsigned long) blk_ahci_ctba_iova(ahci, i, 0),
	       blk_ahci_prdtl(ahci, i, 0));

	/* AHCI interrupt still not handled by AST. */
	evtwait(ahci->evt);
	evtclear(ahci->evt);
	p->is = -1;
	assert(deoi(ahci->d, ahci->irq) == 0);
	p->ie = -1;

	/* Initialisation done. Get IDENTIFY data. */
	ret = ahci_port_get_blkinfo(ahci, i, &blkinfo);
	if (ret) {
		AHCI_PORT_ERR(ahci, i, "Could not get block info. Disabling port");
		goto err;
	}

	/* Register disk device */
	struct blk_ahci_opq *aopq = malloc(sizeof(*aopq));
	if (aopq == NULL) {
		AHCI_PORT_ERR(ahci, i, "Could not allocate AHCI opq for blk. Disabling port");
		goto err;
	}
	aopq->ahci = ahci;
	aopq->port = i;
	name = squoze_sprintf(0, "%s%d", unsquoze_inline(ahci->basename).str, i);
	blk_add(name, ahci->devid, &blkinfo, &ahci_blkops, (void *)aopq);
	return;

err:
	p->cmd &= ~PX_CMD_FRE;
	while (p->cmd & PX_CMD_FR) lwt_pause();

	p->cmd &= ~PX_CMD_ST;
	while (p->cmd & PX_CMD_CR) lwt_pause();
}

static int ahci_port_present(struct blk_ahci *ahci, unsigned i)
{
	volatile struct blk_ahci_hw_port *p = ahci->hw_ports + i;

	uint32_t ssts = p->ssts;
	unsigned det = bitfld_get(ssts, PX_SSTS_DET);
	unsigned ipm = bitfld_get(ssts, PX_SSTS_IPM);
	unsigned spd = bitfld_get(ssts, PX_SSTS_SPD);

	AHCI_PORT_LOG(ahci, i, "status: %s %s %s",
	       det == PX_SSTS_DET_P ? "PRESENT" :
	       det == PX_SSTS_DET_D ? "NO-PHY" :
	       det == PX_SSTS_DET_E ? "EMPTY" : "UNKNOWN",
	       spd == PX_SSTS_SPD_G1 ? "GEN1" :
	       spd == PX_SSTS_SPD_G2 ? "GEN2" :
	       spd == PX_SSTS_SPD_G3 ? "GEN3" : "",
	       ipm == PX_SSTS_IPM_A ? "ACTIVE" :
	       ipm == PX_SSTS_IPM_P ? "PARTIAL" :
	       ipm == PX_SSTS_IPM_S ? "SLUMBER" :
	       ipm == PX_SSTS_IPM_D ? "DEVSLEEP" : "");

	if (bitfld_get(ssts, PX_SSTS_DET) != PX_SSTS_DET_P)
		return 0;

	if (bitfld_get(ssts, PX_SSTS_IPM) != PX_SSTS_IPM_A)
		return 0;

	return 1;
}

static void __ahci_intr(void *arg)
{
	struct blk_ahci *ahci = (struct blk_ahci *)arg;

	while (ahci->ports_impl & ahci->hw_hba->is) {
		int i;
		uint32_t pa = ahci->ports_impl & ahci->hw_hba->is;

		for (i = 0; i < 32; i++)
			if (pa & (1 << i)) {
				ahci_port_intr(ahci, i);
				ahci->hw_hba->is = (1 << i);
			}
	}

	deoi(ahci->d, ahci->irq);
}

static void ahci_reset(struct blk_ahci *ahci)
{
	/* Reset */
	ahci->hw_hba->ghc |= GHC_AE | GHC_HR;

	/* Wait for reset to happen */
  	while (ahci->hw_hba->ghc & GHC_HR) lwt_pause();

	/* Enable interrupts */
    	ahci->hw_hba->ghc |= GHC_IE;

}

static void ahci_print_info(struct blk_ahci *ahci)
{
	uint32_t cap = ahci->hw_hba->cap;

	AHCI_LOG(ahci, "HBA capabilities: NP:%d NCS:%d %s%s%s%s%s"
	       "%s%s%s%s%s%s%s%s%s%s%s%s",
	       1 + bitfld_get(cap, CAP_NP),
	       1 + bitfld_get(cap, CAP_NCS),
	       cap & CAP_SXS ? "SXS " : "",
	       cap & CAP_EMS ? "EMS " : "",
	       cap & CAP_CCCS ? "CCCS " : "",
	       cap & CAP_PSC ? "PSC " : "",
	       cap & CAP_SSC ? "SSC " : "",
	       cap & CAP_PMD ? "PMD " : "",
	       cap & CAP_FBSS ? "FBSS " : "",
	       cap & CAP_SPM ? "SPM " : "",
	       cap & CAP_SAM ? "SAM " : "",
	       cap & CAP_SCLO ? "SCLO " : "",
	       cap & CAP_SAL ? "SAL " : "",
	       cap & CAP_SALP ? "SALP " : "",
	       cap & CAP_SSS ? "SSS " : "",
	       cap & CAP_SMPS ? "SMPS " : "",
	       cap & CAP_SSNTF ? "SSNTF " : "",
	       cap & CAP_SNCQ ? "SNCQ " : "",
	       cap & CAP_S64A ? "S64A " : "");
}

int blkdrv_ahci_probe(uint64_t nameid, uint64_t basename)
{
	int i, evt, ret, irq;
	DEVICE *d;
	ssize_t len;
	unsigned np;
	uint32_t pi;
	uint64_t base;
	struct dinfo info;
	volatile struct blk_ahci_hw_hba *hba;
	volatile struct blk_ahci_hw_port *ports;
	uint64_t ahci_pciid = squoze("PCI01.06.01");
	struct blk_ahci *ahci = NULL;
	iova_t ahcimem_iova = 0;
	vaddr_t ahcimem_va = 0;

	d = dopen(unsquoze_inline(nameid).str);
	if (d == NULL) {
		ret = -ENOENT;
		goto err;
	}

	ret = dgetinfo(d, &info);
	if (ret) {
		goto err;
	}

	for (i = 0; i < info.ndevids; i++)
		if (info.devids[i] == ahci_pciid)
			break;

	if (i >= info.ndevids) {
		ret = -ENOSYS;
		goto err;
	}

	if (info.nmemsegs < 1) {
		ret = -EINVAL;
		goto err;
	}

	len = dgetmemrange(d, 0, &base);
	if (len < 0) {
		ret = (int) len;
		goto err;
	}

	irq = dgetirq(d, 0);
	if (irq < 0) {
		ret = irq;
		goto err;
	}

	evt = evtalloc();
	ret = dmapirq(d, irq, evt);
	if (ret) {
		goto err;
	}

	hba = (volatile struct blk_ahci_hw_hba *)diomap(d, base, len);
	ports = (volatile struct blk_ahci_hw_port *)((void *)hba + 0x100);

	printf("AHCI %s: HBA mapped at %p\n", unsquoze_inline(nameid).str, (void *)hba);

	if (!(hba->cap & CAP_S64A)) {
		printf("AHCI %s: 64-bit addressing not supported. "
		       "Not using this controller.\n");
		ret = -EINVAL;
		goto err;
	}

	ahcimem_va = vmap_alloc(blk_ahci_memsize(), VFNT_WREXEC);
	if (ahcimem_va == 0) {
		ret = -EFAULT;
		goto err;
	}
	ret = vmmap(ahcimem_va, VM_PROT_RW);
	if (ret)
		goto err;
	memset((void *)ahcimem_va, 0, blk_ahci_memsize());
	ret = dexport(d, (void *)ahcimem_va, blk_ahci_memsize(), &ahcimem_iova);
	if (ret)
		goto err;

	ahci = malloc(sizeof(*ahci));
	if (ahci == NULL) {
		ret = -ENOMEM;
		goto err;
	}
	ahci->d = d;
	ahci->devid = nameid;
	ahci->basename = basename;
	ahci->irq = irq;
	ahci->evt = evt;
	ahci->ports_impl = hba->pi;
	ahci->hw_hba = hba;
	ahci->hw_ports = ports;
	ahci->mem = (void *)ahcimem_va;
	ahci->mem_iova = ahcimem_iova;

	ahci_print_info(ahci);
	ahci_reset(ahci);

	np = 1 + bitfld_get(ahci->hw_hba->cap, CAP_NP);
	pi = ahci->ports_impl;
	for (i = 0; i < np; i++) {
		if ((pi & 1) && ahci_port_present(ahci, i)) {
			ahci_port_init(ahci, i);
		}
	}

	/* Start async device via intr. */
	evtast(ahci->evt, __ahci_intr, (void *)ahci);
#if 0
	int r = 0x55;
	char data[1024];
	int opevt = evtalloc();
	memset(data, 0, 1024);

	ahci_port_blkrd((void *)ahci, data, 1024, 0, 1, opevt, &r);

	evtwait(opevt);
	evtclear(opevt);
	printf("data: %d, %s", r, (char *)data);
#endif

	ret = 0;
err:
	if (ret) {
		if (ahcimem_va)
			(void)vmap_free(ahcimem_va, blk_ahci_memsize());
		if (ahci)
			free(ahci);
		dclose(d);
	}
	return ret;
}

