#include <stdint.h>
#include <mrg.h>
#include <mrg/blk.h>
#include <mrg/blk/part.h>
#include <squoze.h>
#include <stdio.h>

struct part {
	struct blkdisk *blk;
	uint64_t start;
	uint64_t length;
};

int part_blkrd(void *opq, void *data, size_t sz, uint64_t blkid, size_t nblks, int evt, int *res)
{
	struct part *p = (struct part *)opq;

	if ((blkid >= p->length) || (blkid + nblks >= p->length))
		return -EINVAL;

	return blk_read(p->blk, data, sz, p->start + blkid, nblks, evt, res);
}

int part_blkwr(void *opq, uint64_t blkid, size_t nblks, void *data, size_t sz, int evt, int *res)
{
	struct part *p = (struct part *)opq;

	if ((blkid >= p->length) || (blkid + nblks >= p->length))
		return -EINVAL;

	return blk_write(p->blk, p->start + blkid, nblks, data, sz, evt, res);
}

static struct blkops part_blkops = {
	.blkrd = part_blkrd,
	.blkwr = part_blkwr,
};


void partadd(unsigned pno, uint64_t blkdev, uint64_t start, uint64_t len)
{
	uint64_t name;
	struct blkdisk *blk;
	struct blkinfo blki, info;
	struct part *p;

	p = malloc(sizeof(*p));
	if (p == NULL) {
		printf("libpart: EOM");
		return;
	}

	blk = blk_open(blkdev);
	if (blk == NULL) {
		printf("libpart: %s not found", unsquoze_inline(blkdev).str);
		return;
	}

	p->blk = blk;
	p->start = start;
	p->length = len;

	blk_info(blk, &blki);
	info.blksz = blki.blksz;
	info.blkno = len;

	name = squoze_sprintf(blkdev, "p%d", pno);

	blk_add(name, blkdev, &info, &part_blkops, (void *)p);
}

int gpt_scan(uint64_t blkid)
{
	/* TODO */
	return 0;
}

#define is_mbr(_s) (((_s)[510] == 0x55) && ((_s)[511] == 0xaa))

int mbr_scan(uint64_t blkid)
{
	int ret;
	uint32_t ps, pl;
	uint8_t sector[512];
	struct blkdisk *blk;
	int evt = evtalloc();

	blk = blk_open(blkid);
	if (blk == NULL) {
		printf("libpart: %s not found", unsquoze_inline(blkid).str);
		return 0;
	}

	if (blk_read(blk, sector, 512, 0, 1, evt, &ret))
		return 0;
	evtwait(evt);
	evtclear(evt);
	if (ret)
		return ret;

	/* Check it's MBR */
	if (!is_mbr(sector))
		return 0;

	/* Partition 1 */
	ps = *(uint32_t *)(sector + 446 + 8);
	pl = *(uint32_t *)(sector + 446 + 12);
	if (ps != 0 & pl != 0)
		partadd(0, blkid, ps, pl);

	/* Partition 2 */
	ps = *(uint32_t *)(sector + 462 + 8);
	pl = *(uint32_t *)(sector + 462 + 12);
	if (ps != 0 & pl != 0)
		partadd(1, blkid, ps, pl);

	/* Partition 3 */
	ps = *(uint32_t *)(sector + 478 + 8);
	pl = *(uint32_t *)(sector + 478 + 12);
	if (ps != 0 & pl != 0)
		partadd(2, blkid, ps, pl);

	/* Partition 4 */
	ps = *(uint32_t *)(sector + 494 + 8);
	pl = *(uint32_t *)(sector + 494 + 12);
	if (ps != 0 & pl != 0)
		partadd(3, blkid, ps, pl);

	blk_close(blk);
	evtfree(evt);
	return 1;
}

void part_scan(uint64_t blkid)
{
	if (!gpt_scan(blkid))
		mbr_scan(blkid);
}
