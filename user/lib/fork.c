#include <env.h>
#include <lib.h>
#include <mmu.h>

/* Overview:
 *   Map the faulting page to a private writable copy.
 *
 * Pre-Condition:
 * 	'va' is the address which led to the TLB Mod exception.
 *
 * Post-Condition:
 *  - Launch a 'user_panic' if 'va' is not a copy-on-write page.
 *  - Otherwise, this handler should map a private writable copy of
 *    the faulting page at the same address.
 */
/* 将故障页映射到私有可写副本 */
static void __attribute__((noreturn)) cow_entry(struct Trapframe *tf) {
	u_int va = tf->cp0_badvaddr;
	u_int perm;

	/* Step 1: Find the 'perm' in which the faulting address 'va' is mapped. */
	/* Hint: Use 'vpt' and 'VPN' to find the page table entry. If the 'perm' doesn't have
	 * 'PTE_COW', launch a 'user_panic'. */
	/* 找到故障地址 va 映射到的页面的 perm 权限 */
	/* 检查 perm 是否含有 PTE_COW 权限 */
	/* 如果没有权限则引发 user_panic */
	/* Exercise 4.13: Your code here. (1/6) */
	perm = vpt[VPN(va)] & 0xfff;
	if (!(perm & PTE_COW)) {
		user_panic("perm doesn't have PTE_COW");
	}

	/* Step 2: Remove 'PTE_COW' from the 'perm', and add 'PTE_D' to it. */
	/* 从 perm 中移除 PTE_COW 并添加 PTE_D 权限 */
	/* Exercise 4.13: Your code here. (2/6) */
	perm = (perm & ~PTE_COW) | PTE_D;

	/* Step 3: Allocate a new page at 'UCOW'. */
	/* Exercise 4.13: Your code here. (3/6) */
	/* 在 UCOW 地址申请新页面 */
	syscall_mem_alloc(0, (void *)UCOW, perm);

	/* Step 4: Copy the content of the faulting page at 'va' to 'UCOW'. */
	/* Hint: 'va' may not be aligned to a page! */
	/* 复制 va 所在故障页的内容到 UCOW 地址 */
	/* Exercise 4.13: Your code here. (4/6) */
	memcpy((void *)UCOW, (void *)ROUNDDOWN(va, PAGE_SIZE), PAGE_SIZE);

	// Step 5: Map the page at 'UCOW' to 'va' with the new 'perm'.
	/* 将 UCOW 地址的页面映射到 va 地址使用新的 perm 权限 */
	/* Exercise 4.13: Your code here. (5/6) */
	syscall_mem_map(0, (void *)UCOW, 0, (void *)va, perm);

	// Step 6: Unmap the page at 'UCOW'.
	/* 取消 UCOW 的页面映射 */
	/* Exercise 4.13: Your code here. (6/6) */
	syscall_mem_unmap(0, (void *)UCOW);

	// Step 7: Return to the faulting routine.
	/* 返回到故障进程 */
	int r = syscall_set_trapframe(0, tf);
	user_panic("syscall_set_trapframe returned %d", r);
}

/* Overview:
 *   Grant our child 'envid' access to the virtual page 'vpn' (with address 'vpn' * 'PAGE_SIZE') in
 * our (current env's) address space. 'PTE_COW' should be used to isolate the modifications on
 * unshared memory from a parent and its children.
 *
 * Post-Condition:
 *   If the virtual page 'vpn' has 'PTE_D' and doesn't has 'PTE_LIBRARY', both our original virtual
 *   page and 'envid''s newly-mapped virtual page should be marked 'PTE_COW' and without 'PTE_D',
 *   while the other permission bits are kept.
 *
 *   If not, the newly-mapped virtual page in 'envid' should have the exact same permission as our
 *   original virtual page.
 *
 * Hint:
 *   - 'PTE_LIBRARY' indicates that the page should be shared among a parent and its children.
 *   - A page with 'PTE_LIBRARY' may have 'PTE_D' at the same time, you should handle it correctly.
 *   - You can pass '0' as an 'envid' in arguments of 'syscall_*' to indicate current env because
 *     kernel 'envid2env' converts '0' to 'curenv').
 *   - You should use 'syscall_mem_map', the user space wrapper around 'msyscall' to invoke
 *     'sys_mem_map' in kernel.
 */
/**
 * 允许 envid 对应进程访问当前进程地址空间中页号为 vpn 的虚拟页面
 * PTE_COW 应该被用于将父子进程对非共享内存上的修改隔离开来
*/
static void duppage(u_int envid, u_int vpn) {
	int r;
	u_int addr;
	u_int perm;

	/* Step 1: Get the permission of the page. */
	/* Hint: Use 'vpt' to find the page table entry. */
	/* 获取页面权限 */
	/* 使用 vpt 获取页表项 */
	/* Exercise 4.10: Your code here. (1/2) */
	addr = vpn << PGSHIFT;
	perm = vpt[vpn] & 0xfff;

	/* Step 2: If the page is writable, and not shared with children, and not marked as COW yet,
	 * then map it as copy-on-write, both in the parent (0) and the child (envid). */
	/* Hint: The page should be first mapped to the child before remapped in the parent. (Why?)
	 */
	/* 如果页面可写并且没有和子进程共享并且还没有被设置为 COW 权限 */
	/* 则将其以 copy-on-right 权限映射 */
	/* 在父进程和子进程中都是如此 */
	/* 注意：应该先在子进程中映射再在父进程中映射 */
	/* Exercise 4.10: Your code here. (2/2) */
	int flag = 0;
	if ((perm & PTE_D) && !(perm & PTE_LIBRARY)) {
		perm = (perm & (~PTE_D)) | (PTE_COW);
		flag = 1;
	}

	syscall_mem_map(0, addr, envid, addr, perm);

	if (flag) {
		syscall_mem_map(0, addr, 0, addr, perm);
	}
}

/* Overview:
 *   User-level 'fork'. Create a child and then copy our address space.
 *   Set up ours and its TLB Mod user exception entry to 'cow_entry'.
 *
 * Post-Conditon:
 *   Child's 'env' is properly set.
 *
 * Hint:
 *   Use global symbols 'env', 'vpt' and 'vpd'.
 *   Use 'syscall_set_tlb_mod_entry', 'syscall_getenvid', 'syscall_exofork',  and 'duppage'.
 */
/**
 * 用户级 fork 函数
 * 创建一个子进程并复制我们的地址空间
 * 设置我们的进程和子进程的 TLB Mod 用户异常处理项为 cow_entry
*/
int fork(void) {
	u_int child;
	u_int i;

	/* Step 1: Set our TLB Mod user exception entry to 'cow_entry' if not done yet. */
	/* 设置我们的进程的 TLB Mod 用户异常处理项为 cow_entry 如果没有设置过 */
	if (env->env_user_tlb_mod_entry != (u_int)cow_entry) {
		try(syscall_set_tlb_mod_entry(0, cow_entry));
	}

	/* Step 2: Create a child env that's not ready to be scheduled. */
	// Hint: 'env' should always point to the current env itself, so we should fix it to the
	// correct value.
	/* 创建一个没有准备好被调度的子进程 */
	/* env 应该永远指向当前进程自己 */
	/* 所以我们应该将其设置为正确的值 */
	child = syscall_exofork();
	if (child == 0) {
		env = envs + ENVX(syscall_getenvid());
		return 0;
	}

	/* Step 3: Map all mapped pages below 'USTACKTOP' into the child's address space. */
	// Hint: You should use 'duppage'.
	/* 映射所有 USTACKTOP 以下的页面到子进程的地址空间 */
	/* Exercise 4.15: Your code here. (1/2) */
	for (i = 0; i < VPN(USTACKTOP); i++) {
		if ((vpd[i >> 10] & PTE_V) && (vpt[i] & PTE_V)) {
			duppage(child, i);
		}
	}

	/* Step 4: Set up the child's tlb mod handler and set child's 'env_status' to
	 * 'ENV_RUNNABLE'. */
	/* Hint:
	 *   You may use 'syscall_set_tlb_mod_entry' and 'syscall_set_env_status'
	 *   Child's TLB Mod user exception entry should handle COW, so set it to 'cow_entry'
	 */
	/* 设置子进程的 TLB Mod 处理函数并且将子进程的状态设置为可运行 */
	/* Exercise 4.15: Your code here. (2/2) */
	try(syscall_set_tlb_mod_entry(child, cow_entry));
	try(syscall_set_env_status(child, ENV_RUNNABLE));

	return child;
}
