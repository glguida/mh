#ifndef __blk_h
#define __blk_h

struct blkinfo {
	size_t blksz;
	uint64_t blkno;
};

struct blkops {
	int      (*blkrd)(void *opq, void *data, size_t sz, uint64_t blkid, size_t nblks, int evt, int *res);
	int      (*blkwr)(void *opq, uint64_t blkid, size_t nblks, void *data, size_t sz, int evt, int *res);
};

struct blkdisk;

void blk_add(uint64_t disk, uint64_t dev, struct blkinfo *info,  const struct blkops *ops,  void *opsopq);
void blk_del(uint64_t disk);

uint64_t blk_iter(uint64_t cur);

struct blkdisk *blk_open(uint64_t disk);
int blk_info(struct blkdisk *dk, struct blkinfo *info);
int blk_read(struct blkdisk *dk, void *addr, size_t sz, uint64_t blkid, size_t blkno, int evt, int *retp);
int blk_write(struct blkdisk *dk, uint64_t blkid, size_t blkno, void *addr, size_t sz, int evt, int *retp);
void blk_close(struct blkdisk *dk);


#endif
