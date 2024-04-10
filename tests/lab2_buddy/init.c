#include <buddy.h>

void mips_init(u_int argc, char **argv, char **penv, u_int ram_low_size) {
	mips_detect_memory(ram_low_size);
	mips_vm_init();
	page_init();
	buddy_init();
	printk("buddy_init succeeded!\n");

	struct Page *pp1, *pp2;

	assert(buddy_alloc(4096, &pp1) == 1); // successfully allocated
	assert(buddy_alloc(4096, &pp2) == 1); // successfully allocated
	assert(pp2 == pp1 + 1);		      // they are buddy pages

	buddy_free(pp1, 1);
	buddy_free(pp2, 1); // pages should be merged

	assert(buddy_alloc(8192, &pp2) == 2); // successfully allocated

	buddy_free(pp2, 2);
	printk("Buddy page test passed!\n");
	halt();
}
