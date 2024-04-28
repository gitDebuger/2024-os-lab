#include <bitops.h>
#include <env.h>
#include <pmap.h>
/**
 * | 31 ----> 13 | 12 ----> 8 | 7 ----> 0 |
 * |     VPN     |      0     |    ASID   |
 * EntryHi Register (TLB Key Fields)
 * VPN：TLB缺失时由硬件自动填充为虚页号，需要填充或检索TLB表项时由软件填充为虚拟地址
 * ASID：用于区分不同的地址空间。同一虚拟地址在不同的地址空间通常映射到不同的物理地址。
 * Address Space IDentifier
*/
/**
 * | 31 -> 30 | 29 ----> 6 | 5 --> 3 | 2 | 1 | 0 |
 * |    0     |    PFN     |    C    | D | V | G |
 * EntryLo Register (TLB Data Fields)
 * PFN——Physical Frame Number
*/

/* Lab 2 Key Code "tlb_invalidate" */
/* Overview:
 *   Invalidate the TLB entry with specified 'asid' and virtual address 'va'.
 *
 * Hint:
 *   Construct a new Entry HI and call 'tlb_out' to flush TLB.
 *   'tlb_out' is defined in mm/tlb_asm.S
 */
/**
 * 在更新页表项之后需要将TLB中表项无效化
 * 这样在下一次访问该虚拟地址时会触发TLB重填异常
 * 从而操作系统对TLB进行重填
*/
void tlb_invalidate(u_int asid, u_long va) {
	tlb_out((va & ~GENMASK(PGSHIFT, 0)) | (asid & (NASID - 1)));
}
/* End of Key Code "tlb_invalidate" */

/**
 * 以为此虚拟地址申请一个物理页面并将虚拟地址映射到该物理页面
 * 即进行被动页面分配
*/
static void passive_alloc(u_int va, Pde *pgdir, u_int asid) {
	struct Page *p = NULL;

	if (va < UTEMP) {
		panic("address too low");
	}

	if (va >= USTACKTOP && va < USTACKTOP + PAGE_SIZE) {
		panic("invalid memory");
	}

	if (va >= UENVS && va < UPAGES) {
		panic("envs zone");
	}

	if (va >= UPAGES && va < UVPT) {
		panic("pages zone");
	}

	if (va >= ULIM) {
		panic("kernel address");
	}

	panic_on(page_alloc(&p));
	panic_on(page_insert(pgdir, asid, p, PTE_ADDR(va), (va >= UVPT && va < ULIM) ? 0 : PTE_D));
}

/* Overview:
 *  Refill TLB.
 */
/**
 * 进行TLB重填
*/
void _do_tlb_refill(u_long *pentrylo, u_int va, u_int asid) {
	/* 首先进行TLB无效化 */
	tlb_invalidate(asid, va);
	Pte *ppte;
	/* Hints:
	 *  Invoke 'page_lookup' repeatedly in a loop to find the page table entry '*ppte'
	 * associated with the virtual address 'va' in the current address space 'cur_pgdir'.
	 *
	 *  **While** 'page_lookup' returns 'NULL', indicating that the '*ppte' could not be found,
	 *  allocate a new page using 'passive_alloc' until 'page_lookup' succeeds.
	 */

	/* Exercise 2.9: Your code here. */
	/* The variable cur_pgdir is in the start of pmap.c?! */
	/* 循环查找是否存在va对应的页面 */
	/* 如果不存在则分配页面 */
	while (page_lookup(cur_pgdir, va, &ppte) == NULL) {
		passive_alloc(va, cur_pgdir, asid);
	}

	ppte = (Pte *)((u_long)ppte & ~0x7);
	pentrylo[0] = ppte[0] >> 6;
	pentrylo[1] = ppte[1] >> 6;
}

#if !defined(LAB) || LAB >= 4
/* Overview:
 *   This is the TLB Mod exception handler in kernel.
 *   Our kernel allows user programs to handle TLB Mod exception in user mode, so we copy its
 *   context 'tf' into UXSTACK and modify the EPC to the registered user exception entry.
 *
 * Hints:
 *   'env_user_tlb_mod_entry' is the user space entry registered using
 *   'sys_set_user_tlb_mod_entry'.
 *
 *   The user entry should handle this TLB Mod exception and restore the context.
 */
void do_tlb_mod(struct Trapframe *tf) {
	struct Trapframe tmp_tf = *tf;

	if (tf->regs[29] < USTACKTOP || tf->regs[29] >= UXSTACKTOP) {
		tf->regs[29] = UXSTACKTOP;
	}
	tf->regs[29] -= sizeof(struct Trapframe);
	*(struct Trapframe *)tf->regs[29] = tmp_tf;
	Pte *pte;
	page_lookup(cur_pgdir, tf->cp0_badvaddr, &pte);
	if (curenv->env_user_tlb_mod_entry) {
		tf->regs[4] = tf->regs[29];
		tf->regs[29] -= sizeof(tf->regs[4]);
		// Hint: Set 'cp0_epc' in the context 'tf' to 'curenv->env_user_tlb_mod_entry'.
		/* Exercise 4.11: Your code here. */
		tf->cp0_epc = curenv->env_user_tlb_mod_entry;
	} else {
		panic("TLB Mod but no user handler registered");
	}
}
#endif
