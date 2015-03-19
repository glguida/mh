#ifndef __ukern_vmap_h
#define __ukern_vmap_h

#define VFNT_INVALID 0
#define VFNT_FREE    1
#define VFNT_MAP     2

vaddr_t vmap_alloc(size_t size, uint8_t type);
void vmap_free(vaddr_t va, size_t size);
void vmap_info(vaddr_t va, vaddr_t * start, size_t * size, uint8_t * type);
void vmap_init(void);

void *kvmap(paddr_t addr, size_t size);
void kvunmap(vaddr_t vaddr, size_t size);

#endif
