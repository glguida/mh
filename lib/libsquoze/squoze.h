#ifndef _squoze_h_
#define _squoze_h_

uint64_t squoze(char *string);
char *unsquoze(uint64_t enc);

size_t unsquozelen(uint64_t enc, size_t len, char *string);

#endif
