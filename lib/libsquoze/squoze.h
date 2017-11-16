#ifndef __squoze_h
#define __squoze_h

typedef struct {
	char str[13];
} squoze_fixed_t;

size_t unsquozelen(uint64_t enc, size_t len, char *string);

uint64_t squoze(char *string);
uint64_t squoze_append(uint64_t sqz, char *string);
char *unsquoze(uint64_t enc);
void unsquoze_fixed(uint64_t enc, squoze_fixed_t *fixed);
squoze_fixed_t unsquoze_inline(uint64_t enc);
uint64_t squoze_sprintf(uint64_t sqz, char *fmt, ...);

#endif
