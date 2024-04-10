#include <mmu.h>
#include <pmap.h>
#include <print.h>

void test_page_filter() {
	struct Page *p;
	assert(page_alloc(&p) == 0);
	Pde *pgdir = (Pde *)page2kva(p);
	u_int va[6] = {UTEXT,
		       UTEXT + PAGE_SIZE,
		       UTEXT + 1024 * PAGE_SIZE,
		       UTEXT + 1025 * PAGE_SIZE,
		       UTEXT + 2048 * PAGE_SIZE,
		       UTEXT + 2049 * PAGE_SIZE};
	u_int perm[6] = {PTE_V,		  PTE_V | PTE_D,	   PTE_V | PTE_G,
			 PTE_V | PTE_COW, PTE_V | PTE_COW | PTE_D, PTE_V | PTE_D | PTE_G};

	struct Page *pp1, *pp2, *pp3;
	assert(page_alloc(&pp1) == 0);
	assert(page_insert(pgdir, 0, pp1, va[0], perm[0]) == 0);

	assert(page_alloc(&pp2) == 0);
	assert(page_insert(pgdir, 0, pp2, va[1], perm[1]) == 0);
	assert(page_insert(pgdir, 0, pp2, va[2], perm[2]) == 0);

	assert(page_alloc(&pp3) == 0);
	assert(page_insert(pgdir, 0, pp3, va[3], perm[3]) == 0);
	assert(page_insert(pgdir, 0, pp1, va[4], perm[4]) == 0);
	assert(page_insert(pgdir, 0, pp1, va[5], perm[5]) == 0);

	int r = page_filter(pgdir, UTEXT, UTEXT + 2050 * PAGE_SIZE, 2);
	assert(r == 5);
	printk("test_page_filter succeeded!\n");
}

void mips_init(u_int argc, char **argv, char **penv, u_int ram_low_size) {
	mips_detect_memory(ram_low_size);
	mips_vm_init();
	page_init();
	test_page_filter();
	halt();
}
