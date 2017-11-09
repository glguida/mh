#include <uk/types.h>
#include <uk/assert.h>
#include <machine/uk/pmap.h>
#include <uk/kern.h>

/*
 * Quick cross copy with temporal external mapping
 */

int xcopy_from(vaddr_t va, struct thread *th, vaddr_t xva, size_t sz)
{
	int ret;
	pfn_t pfn;
	l1e_t extl1e;
	size_t todo;
	

	while (sz) {
		todo = sz < PAGE_SIZE - (xva & PAGE_MASK) ? sz : PAGE_SIZE - (xva & PAGE_MASK);

		pmap_hmap(th->pmap, xva);
		ret = copy_to_user(va, (void *)(KVA_HYPER + (xva & PAGE_MASK)), todo);
		pmap_hunmap(th->pmap);

		if (ret != 0)
			break;

		xva += todo;
		va += todo;
		sz -= todo;
	}

	return ret;
}

int xcopy_to(struct thread *th, vaddr_t xva, vaddr_t va, size_t sz)
{
	int ret;
	pfn_t pfn;
	l1e_t extl1e;
	size_t todo;
	

	while (sz) {
		todo = sz < PAGE_SIZE - (xva & PAGE_MASK) ? sz : PAGE_SIZE - (xva & PAGE_MASK);

		pmap_hmap(th->pmap, xva);
		ret = copy_from_user((void *)(KVA_HYPER + (xva & PAGE_MASK)), va, todo);
		pmap_hunmap(th->pmap);

		if (ret != 0)
			break;

		xva += todo;
		va += todo;
		sz -= todo;
	}

	return ret;
}
