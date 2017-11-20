#ifndef __part_h
#define __part_h

int gpt_scan(uint64_t blkid);
int mbr_scan(uint64_t blkid);
void part_scan(uint64_t blkid);

#endif
