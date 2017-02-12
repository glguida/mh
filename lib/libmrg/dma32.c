#include <sys/types.h>
#include <machine/vmparam.h>
#include <microkernel.h>
#include <mrg.h>

void *dma32_alloc(size_t len)
{
	int ret;
	vaddr_t i, j, va;

	va = vmap_alloc(len, VFNT_MEM32);
	if (va == 0) {
		/* errno: set RET */
		return NULL;
	}

	for (i = 0; i < len; i += PAGE_SIZE) {
		ret = vmmap32(va + i, VM_PROT_RW);
		if (ret) {
			for (j = 0; j < i; j++)
				vmunmap(va + j);
			vmap_free(va, len);
			/* errno: set RET */
			return NULL;
		}
	}
	return (void *)va;
}

void dma32_free(void *vaddr, size_t len)
{
	int ret;
	vaddr_t i, j, va;

	va = (vaddr_t)(uintptr_t)vaddr;
	for (j = 0; j < i; j++)
		vmunmap(va + j);
	vmap_free(va, len);
	/* errno: set RET */
}
