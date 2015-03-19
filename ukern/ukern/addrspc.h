#ifndef _ukern_addrspc_h
#define _ukern_addrspc_h

#include <machine/uk/pmap.h>
#include <uk/rbtree.h>

struct pager;

struct vrange {
	struct rb_node rb_node;
	struct addrspc *as;
	size_t start;
	size_t end;
	pmap_prot_t prot;
	struct pager *pager;
	void *pager_opq;
};

struct pager {
	int (*__pgin) (struct vrange * vr, size_t off, pmap_fault_t fault,
		       void *opq);
	 size_t(*__pgout) (struct vrange * vr, size_t sz, void *opq);
};

struct addrspc {
	struct pmap *pmap;

	lock_t lock;
	rb_tree_t vranges;
};

void addrspc_init(void);
struct addrspc *addrspc_boot(struct pmap *pmap);
struct addrspc *addrspc_create(void);
int addrspc_add_range(struct addrspc *as, vaddr_t start, size_t size,
		      pmap_prot_t, struct pager *pager, void *opq);
int addrspc_pagefault(vaddr_t va, pmap_fault_t fault);

static inline void addrspc_switch(struct addrspc *as)
{

	pmap_switch(as->pmap);
}

#endif
