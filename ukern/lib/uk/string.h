#ifndef __uk_string_h
#define __uk_string_h

void *memset(void *b, int c, size_t len);
void *memcpy(void *d, void *s, size_t len);
int memcmp(void *s1, void *s2, size_t len);

int fls(int);
int ffs(int);

uint64_t squoze(char *string);
size_t unsquozelen(uint64_t enc, size_t len, char *string);

#endif
