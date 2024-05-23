#include <env.h>
#include <io.h>
#include <mmu.h>
#include <pmap.h>
#include <printk.h>
#include <sched.h>
#include <syscall.h>

/* 定义在 kern/env.c 中的当前进程控制块 */
extern struct Env *curenv;

/* Overview:
 * 	This function is used to print a character on screen.
 *
 * Pre-Condition:
 * 	`c` is the character you want to print.
 */
/* 用于打印一个字符到屏幕 */
void sys_putchar(int c) {
	printcharc((char)c);
	return;
}

/* Overview:
 * 	This function is used to print a string of bytes on screen.
 *
 * Pre-Condition:
 * 	`s` is base address of the string, and `num` is length of the string.
 */
/* 用于打印给定长度的字符串到屏幕 */
int sys_print_cons(const void *s, u_int num) {
	if (((u_int)s + num) > UTOP || ((u_int)s) >= UTOP || (s > s + num)) {
		return -E_INVAL;
	}
	u_int i;
	for (i = 0; i < num; i++) {
		printcharc(((char *)s)[i]);
	}
	return 0;
}

/* Overview:
 *	This function provides the environment id of current process.
 *
 * Post-Condition:
 * 	return the current environment id
 */
/* 获取当前进程的进程号 */
u_int sys_getenvid(void) {
	return curenv->env_id;
}

/* Overview:
 *   Give up remaining CPU time slice for 'curenv'.
 *
 * Post-Condition:
 *   Another env is scheduled.
 *
 * Hint:
 *   This function will never return.
 */
// void sys_yield(void);
/* 放弃当前进程剩余的 CPU 时间片 */
/* 另一个进程将被调度 */
/* 这个函数永远不会返回 */
void __attribute__((noreturn)) sys_yield(void) {
	// Hint: Just use 'schedule' with 'yield' set.
	/* Exercise 4.7: Your code here. */
	schedule(1);
}

/* Overview:
 * 	This function is used to destroy the current environment.
 *
 * Pre-Condition:
 * 	The parameter `envid` must be the environment id of a
 * process, which is either a child of the caller of this function
 * or the caller itself.
 *
 * Post-Condition:
 *  Returns 0 on success.
 *  Returns the original error if underlying calls fail.
 */
/* 用于销毁进程控制块 */
/* 参数 envid 必须是某个进程的进程号 */
/* 同时被销毁的进程或者是调用者的子进程或者是调用者本身 */
/* 返回 0 如果销毁成功否则返回原始错误 */
int sys_env_destroy(u_int envid) {
	struct Env *e;
	try(envid2env(envid, &e, 1));

	printk("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

/* Overview:
 *   Register the entry of user space TLB Mod handler of 'envid'.
 *
 * Post-Condition:
 *   The 'envid''s TLB Mod exception handler entry will be set to 'func'.
 *   Returns 0 on success.
 *   Returns the original error if underlying calls fail.
 */
/* 注册 envid 用户空间的 TLB Mod 处理程序入口 */
/* envid 对应的进程 TLB Mod 异常处理程序入口将被设置为 func */
/* 返回 0 如果成功 */
/* 返回原始错误如果调用失败 */
int sys_set_tlb_mod_entry(u_int envid, u_int func) {
	struct Env *env;

	/* Step 1: Convert the envid to its corresponding 'struct Env *' using 'envid2env'. */
	/* 使用 envid2env 将 envid 转换为对应的进程控制块结构体 */
	/* Exercise 4.12: Your code here. (1/2) */
	try(envid2env(envid, &env, 1));

	/* Step 2: Set its 'env_user_tlb_mod_entry' to 'func'. */
	/* 将进程控制块结构体中的 env_user_tlb_mod_entry 字段设置为 func */
	/* Exercise 4.12: Your code here. (2/2) */
	env->env_user_tlb_mod_entry = func;

	return 0;
}

/* Overview:
 *   Check 'va' is illegal or not, according to include/mmu.h
 */
/* 依据 include/mmu.h 中的地址空间布局检验 va 是否非法 */
static inline int is_illegal_va(u_long va) {
	return va < UTEMP || va >= UTOP;
}

/* 依据 include/mmu.h 中的地址空间布局检验 va 范围是否非法 */
static inline int is_illegal_va_range(u_long va, u_int len) {
	if (len == 0) {
		return 0;
	}
	return va + len < va || va < UTEMP || va + len > UTOP;
}

/* Overview:
 *   Allocate a physical page and map 'va' to it with 'perm' in the address space of 'envid'.
 *   If 'va' is already mapped, that original page is sliently unmapped.
 *   'envid2env' should be used with 'checkperm' set, like in most syscalls, to ensure the target is
 * either the caller or its child.
 *
 * Post-Condition:
 *   Return 0 on success.
 *   Return -E_BAD_ENV: 'checkperm' of 'envid2env' fails for 'envid'.
 *   Return -E_INVAL:   'va' is illegal (should be checked using 'is_illegal_va').
 *   Return the original error: underlying calls fail (you can use 'try' macro).
 *
 * Hint:
 *   You may want to use the following functions:
 *   'envid2env', 'page_alloc', 'page_insert', 'try' (macro)
 */
/**
 * 申请一个物理页框并使用权限 perm 将 envid 的地址空间中的虚拟地址 va 映射到该物理页框
 * 如果 va 已经被映射那么原始页面将被静默地取消映射
 * 调用 envid2env 时 checkperm 需要被设置以确保目标是调用者本身或者它的子进程
 * @return 0 成功
 * -E_BAD_ENV 调用 envid2env 时 checkperm 失败
 * -E_INVAL 地址 va 非法——使用 is_illegal_va 检查
 * 原始错误 底层调用失败——使用 try 宏
*/
int sys_mem_alloc(u_int envid, u_int va, u_int perm) {
	struct Env *env;
	struct Page *pp;

	/* Step 1: Check if 'va' is a legal user virtual address using 'is_illegal_va'. */
	/* 使用 is_illegal_va 检查 va 是否为合法的用户虚拟地址 */
	/* Exercise 4.4: Your code here. (1/3) */
	if (is_illegal_va(va)) {
		return -E_INVAL;
	}

	/* Step 2: Convert the envid to its corresponding 'struct Env *' using 'envid2env'. */
	/* Hint: **Always** validate the permission in syscalls! */
	/* 将 env_id 转换为对应的进程控制块结构体使用 envid2env 函数 */
	/* 注意：需要将 checkperm 设置为有效 */
	/* Exercise 4.4: Your code here. (2/3) */
	try(envid2env(envid, &env, 1));

	/* Step 3: Allocate a physical page using 'page_alloc'. */
	/* 使用 page_alloc 申请一张物理页框 */
	/* Exercise 4.4: Your code here. (3/3) */
	try(page_alloc(&pp));

	/* Step 4: Map the allocated page at 'va' with permission 'perm' using 'page_insert'. */
	/* 使用 page_insert 将 va 以权限 perm 映射到刚才申请的页面 */
	return page_insert(env->env_pgdir, env->env_asid, pp, va, perm);
}

/* Overview:
 *   Find the physical page mapped at 'srcva' in the address space of env 'srcid', and map 'dstid''s
 *   'dstva' to it with 'perm'.
 *
 * Post-Condition:
 *   Return 0 on success.
 *   Return -E_BAD_ENV: 'checkperm' of 'envid2env' fails for 'srcid' or 'dstid'.
 *   Return -E_INVAL: 'srcva' or 'dstva' is illegal, or 'srcva' is unmapped in 'srcid'.
 *   Return the original error: underlying calls fail.
 *
 * Hint:
 *   You may want to use the following functions:
 *   'envid2env', 'page_lookup', 'page_insert'
 */
/**
 * 在 srcid 对应的进程地址空间中找到映射到 srcva 的物理页框
 * 然后将 dstid 对应的进程地址空间中的 dstva 使用 perm 权限映射到该物理页框
 * @return 0 成功
 * -E_BAD_ENV 调用 envid2env 时 srcid 或 dstid 权限检查失败
 * -E_INVAL srcva 或 dstva 非法或者 srcva 在 srcid 中解除映射
 * 原始错误 底层调用失败
*/
int sys_mem_map(u_int srcid, u_int srcva, u_int dstid, u_int dstva, u_int perm) {
	struct Env *srcenv;
	struct Env *dstenv;
	struct Page *pp;

	/* Step 1: Check if 'srcva' and 'dstva' are legal user virtual addresses using
	 * 'is_illegal_va'. */
	/* 使用 is_illegal_va 检查 srcva 和 dstva 是否为合法的用户虚拟地址 */
	/* Exercise 4.5: Your code here. (1/4) */
	if (is_illegal_va(srcva) || is_illegal_va(dstva)) {
		return -E_INVAL;
	}

	/* Step 2: Convert the 'srcid' to its corresponding 'struct Env *' using 'envid2env'. */
	/* Exercise 4.5: Your code here. (2/4) */
	/* 将 srcid 转换为对应的进程控制块 */
	try(envid2env(srcid, &srcenv, 1));

	/* Step 3: Convert the 'dstid' to its corresponding 'struct Env *' using 'envid2env'. */
	/* Exercise 4.5: Your code here. (3/4) */
	/* 将 dstid 转换为对应的进程控制块 */
	try(envid2env(dstid, &dstenv, 1));

	/* Step 4: Find the physical page mapped at 'srcva' in the address space of 'srcid'. */
	/* Return -E_INVAL if 'srcva' is not mapped. */
	/* 在 srcid 对应进程的地址空间中寻找 srcva 映射到的物理页框 */
	/* 返回 -E_INVAL 如果 srcva 没有被映射 */
	/* Exercise 4.5: Your code here. (4/4) */
	if ((pp = page_lookup(srcenv->env_pgdir, srcva, NULL)) == NULL) {
		return -E_INVAL;
	}

	/* Step 5: Map the physical page at 'dstva' in the address space of 'dstid'. */
	/* 在 dstid 对应进程的地址空间中映射 dstva 到该物理页框 */
	return page_insert(dstenv->env_pgdir, dstenv->env_asid, pp, dstva, perm);
}

/* Overview:
 *   Unmap the physical page mapped at 'va' in the address space of 'envid'.
 *   If no physical page is mapped there, this function silently succeeds.
 *
 * Post-Condition:
 *   Return 0 on success.
 *   Return -E_BAD_ENV: 'checkperm' of 'envid2env' fails for 'envid'.
 *   Return -E_INVAL:   'va' is illegal.
 *   Return the original error when underlying calls fail.
 */
/**
 * 取消 envid 对应进程地址空间中 va 到物理页框的映射
 * 如果没有对应的映射则该函数默认成功
*/
int sys_mem_unmap(u_int envid, u_int va) {
	struct Env *e;

	/* Step 1: Check if 'va' is a legal user virtual address using 'is_illegal_va'. */
	/* 检查 va 是否合法 */
	/* Exercise 4.6: Your code here. (1/2) */
	if (is_illegal_va(va)) {
		return -E_INVAL;
	}

	/* Step 2: Convert the envid to its corresponding 'struct Env *' using 'envid2env'. */
	/* 将 envid 转换为对应的进程控制块 */
	/* Exercise 4.6: Your code here. (2/2) */
	try(envid2env(envid, &e, 1));

	/* Step 3: Unmap the physical page at 'va' in the address space of 'envid'. */
	/* 取消 envid 对应进程地址空间中 va 对物理页框的映射 */
	page_remove(e->env_pgdir, e->env_asid, va);
	return 0;
}

/* Overview:
 *   Allocate a new env as a child of 'curenv'.
 *
 * Post-Condition:
 *   Returns the child's envid on success, and
 *   - The new env's 'env_tf' is copied from the kernel stack, except for $v0 set to 0 to indicate
 *     the return value in child.
 *   - The new env's 'env_status' is set to 'ENV_NOT_RUNNABLE'.
 *   - The new env's 'env_pri' is copied from 'curenv'.
 *   Returns the original error if underlying calls fail.
 *
 * Hint:
 *   This syscall works as an essential step in user-space 'fork' and 'spawn'.
 */
/**
 * 申请一个新的进程作为当前进程的子进程
 * 返回子进程的进程号如果成功并且：
 * 1. 子进程的陷入帧从内核栈中拷贝过来
 * 同时将子进程的 $v0 寄存器设置为 0 用以标识这是子进程
 * 2. 子进程的进程状态设置为不可运行
 * 3. 子进程的优先级从当前进程拷贝过来
 * 返回原始错误如果底层调用失败
 * 注意：该系统调用在用户空间的 fork 函数和 spawn 函数中是重要的一步
*/
int sys_exofork(void) {
	struct Env *e;

	/* Step 1: Allocate a new env using 'env_alloc'. */
	/* 申请一个新的进程控制块 */
	/* Exercise 4.9: Your code here. (1/4) */
	try(env_alloc(&e, curenv->env_id));

	/* Step 2: Copy the current Trapframe below 'KSTACKTOP' to the new env's 'env_tf'. */
	/* 将 KSTACKTOP 下面的当前陷入帧复制到新进程的陷入帧中 */
	/* Exercise 4.9: Your code here. (2/4) */
	e->env_tf = *((struct Trapframe *)KSTACKTOP - 1);

	/* Step 3: Set the new env's 'env_tf.regs[2]' to 0 to indicate the return value in child. */
	/* Exercise 4.9: Your code here. (3/4) */
	/* 将新进程的 $v0 寄存器设置为 0 用以标识子进程 */
	e->env_tf.regs[2] = 0;

	/* Step 4: Set up the new env's 'env_status' and 'env_pri'.  */
	/* 设置新进程的进程状态和进程优先级 */
	/* Exercise 4.9: Your code here. (4/4) */
	e->env_status = ENV_NOT_RUNNABLE;
	e->env_pri = curenv->env_pri;

	return e->env_id;
}

/* Overview:
 *   Set 'envid''s 'env_status' to 'status' and update 'env_sched_list'.
 *
 * Post-Condition:
 *   Returns 0 on success.
 *   Returns -E_INVAL if 'status' is neither 'ENV_RUNNABLE' nor 'ENV_NOT_RUNNABLE'.
 *   Returns the original error if underlying calls fail.
 *
 * Hint:
 *   The invariant that 'env_sched_list' contains and only contains all runnable envs should be
 *   maintained.
 */
/**
 * 设置 envid 对应进程的进程状态并更新 env_sched_list
 * 该列表包含且仅包含所有应该被保持的可运行进程
*/
int sys_set_env_status(u_int envid, u_int status) {
	struct Env *env;

	/* Step 1: Check if 'status' is valid. */
	/* 检查 status 是否有效 */
	/* Exercise 4.14: Your code here. (1/3) */
	if (status != ENV_RUNNABLE && status != ENV_NOT_RUNNABLE) {
		return -E_INVAL;
	}

	/* Step 2: Convert the envid to its corresponding 'struct Env *' using 'envid2env'. */
	/* 将 envid 转换为对应的进程控制块 */
	/* Exercise 4.14: Your code here. (2/3) */
	try(envid2env(envid, &env, 1));

	/* Step 3: Update 'env_sched_list' if the 'env_status' of 'env' is being changed. */
	/* 更新 env_sched_list 如果进程状态需要被改变 */
	/* Exercise 4.14: Your code here. (3/3) */
	if (env->env_status != ENV_NOT_RUNNABLE && status == ENV_NOT_RUNNABLE) {
		TAILQ_REMOVE(&env_sched_list, env, env_sched_link);
	} else if (env->env_status != ENV_RUNNABLE && status == ENV_RUNNABLE) {
		TAILQ_INSERT_TAIL(&env_sched_list, env, env_sched_link);
	}

	/* Step 4: Set the 'env_status' of 'env'. */
	/* 设置进程状态 */
	env->env_status = status;
	return 0;
}

/* Overview:
 *  Set envid's trap frame to 'tf'.
 *
 * Post-Condition:
 *  The target env's context is set to 'tf'.
 *  Returns 0 on success (except when the 'envid' is the current env, so no value could be
 * returned).
 *  Returns -E_INVAL if the environment cannot be manipulated or 'tf' is invalid.
 *  Returns the original error if other underlying calls fail.
 */
/* 设置进程的陷入帧 */
int sys_set_trapframe(u_int envid, struct Trapframe *tf) {
	/* 检查地址范围是否合法 */
	if (is_illegal_va_range((u_long)tf, sizeof *tf)) {
		return -E_INVAL;
	}
	struct Env *env;
	/* 将 envid 转换为对应的进程控制块 */
	try(envid2env(envid, &env, 1));
	/* 设置进程陷入帧 */
	if (env == curenv) {
		*((struct Trapframe *)KSTACKTOP - 1) = *tf;
		// return `tf->regs[2]` instead of 0, because return value overrides regs[2] on
		// current trapframe.
		return tf->regs[2];
	} else {
		env->env_tf = *tf;
		return 0;
	}
}

/* Overview:
 * 	Kernel panic with message `msg`.
 *
 * Post-Condition:
 * 	This function will halt the system.
 */
/* 带有消息 msg 的内核 panic */
void sys_panic(char *msg) {
	panic("%s", TRUP(msg));
}

/* Overview:
 *   Wait for a message (a value, together with a page if 'dstva' is not 0) from other envs.
 *   'curenv' is blocked until a message is sent.
 *
 * Post-Condition:
 *   Return 0 on success.
 *   Return -E_INVAL: 'dstva' is neither 0 nor a legal address.
 */
/**
 * 等待从另一个进程过来的消息：
 * 包括一个值和一个页面如果 dstva 不是 0
 * 当前进程将被阻塞直到消息送达
*/
int sys_ipc_recv(u_int dstva) {
	/* Step 1: Check if 'dstva' is either zero or a legal address. */
	/* 检查 dstva 或者是 0 或者是合法地址 */
	if (dstva != 0 && is_illegal_va(dstva)) {
		return -E_INVAL;
	}

	/* Step 2: Set 'curenv->env_ipc_recving' to 1. */
	/* 将当前进程设置为可接收消息状态 */
	/* Exercise 4.8: Your code here. (1/8) */
	curenv->env_ipc_recving = 1;

	/* Step 3: Set the value of 'curenv->env_ipc_dstva'. */
	/* 设置当前进程的消息目标地址为 dstva */
	/* Exercise 4.8: Your code here. (2/8) */
	curenv->env_ipc_dstva = dstva;

	/* Step 4: Set the status of 'curenv' to 'ENV_NOT_RUNNABLE' and remove it from
	 * 'env_sched_list'. */
	/* 将当前进程状态设置为不可运行并从 env_sched_list 中移除 */
	/* Exercise 4.8: Your code here. (3/8) */
	curenv->env_status = ENV_NOT_RUNNABLE;
	TAILQ_REMOVE(&env_sched_list, curenv, env_sched_link);

	/* Step 5: Give up the CPU and block until a message is received. */
	/* 放弃 CPU 并阻塞直到消息到达 */
	((struct Trapframe *)KSTACKTOP - 1)->regs[2] = 0;
	schedule(1);
}

/* Overview:
 *   Try to send a 'value' (together with a page if 'srcva' is not 0) to the target env 'envid'.
 *
 * Post-Condition:
 *   Return 0 on success, and the target env is updated as follows:
 *   - 'env_ipc_recving' is set to 0 to block future sends.
 *   - 'env_ipc_from' is set to the sender's envid.
 *   - 'env_ipc_value' is set to the 'value'.
 *   - 'env_status' is set to 'ENV_RUNNABLE' again to recover from 'ipc_recv'.
 *   - if 'srcva' is not NULL, map 'env_ipc_dstva' to the same page mapped at 'srcva' in 'curenv'
 *     with 'perm'.
 *
 *   Return -E_IPC_NOT_RECV if the target has not been waiting for an IPC message with
 *   'sys_ipc_recv'.
 *   Return the original error when underlying calls fail.
 */
/**
 * 尝试发送一个值以及一个页面如果 srcva 非零
 * 到 envid 对应的进程
*/
int sys_ipc_try_send(u_int envid, u_int value, u_int srcva, u_int perm) {
	struct Env *e;
	struct Page *p;

	/* Step 1: Check if 'srcva' is either zero or a legal address. */
	/* 检查 srcva 或者为 0 或者为其他合法地址 */
	/* Exercise 4.8: Your code here. (4/8) */
	if (srcva != 0 && is_illegal_va(srcva)) {
		return -E_INVAL;
	}

	/* Step 2: Convert 'envid' to 'struct Env *e'. */
	/* This is the only syscall where the 'envid2env' should be used with 'checkperm' UNSET,
	 * because the target env is not restricted to 'curenv''s children. */
	/* 将 envid 转换为对应进程控制块 */
	/* 这里不需要设置 checkperm 因为目标进程不一定是当前进程的子进程 */
	/* Exercise 4.8: Your code here. (5/8) */
	try(envid2env(envid, &e, 0));

	/* Step 3: Check if the target is waiting for a message. */
	/* 检查目标是否在等待接收消息 */
	/* Exercise 4.8: Your code here. (6/8) */
	if (!(e->env_ipc_recving)) {
		return -E_IPC_NOT_RECV;
	}

	/* Step 4: Set the target's ipc fields. */
	/* 设置目标的 ipc 域字段 */
	/* 值 value */
	e->env_ipc_value = value;
	/* 消息来源进程号 */
	e->env_ipc_from = curenv->env_id;
	/* 权限 */
	e->env_ipc_perm = PTE_V | perm;
	/* 取消接收状态 */
	e->env_ipc_recving = 0;

	/* Step 5: Set the target's status to 'ENV_RUNNABLE' again and insert it to the tail of
	 * 'env_sched_list'. */
	/* 将目标进程的状态设置为可运行并将其插入 env_sched_list 的尾部 */
	/* Exercise 4.8: Your code here. (7/8) */
	e->env_status = ENV_RUNNABLE;
	TAILQ_INSERT_TAIL(&env_sched_list, e, env_sched_link);

	/* Step 6: If 'srcva' is not zero, map the page at 'srcva' in 'curenv' to 'e->env_ipc_dstva'
	 * in 'e'. */
	/* 如果 srcva 非零则将当前进程的 srcva 对应的页面映射到目标进程的 env_ipc_dstva */
	/* Return -E_INVAL if 'srcva' is not zero and not mapped in 'curenv'. */
	/* 返回 -E_INVAL 如果 srcva 非零并且在当前进程中没有映射 */
	if (srcva != 0) {
		/* Exercise 4.8: Your code here. (8/8) */
		p = page_lookup(curenv->env_pgdir, srcva, NULL);
		if (p == NULL) {
			return -E_INVAL;
		}
		try(page_insert(e->env_pgdir, e->env_asid, p, e->env_ipc_dstva, perm));
	}
	return 0;
}

// XXX: kernel does busy waiting here, blocking all envs
/* 读取一个字符 */
/* 内核处于忙等状态 */
/* 阻塞所有进程 */
int sys_cgetc(void) {
	int ch;
	while ((ch = scancharc()) == 0) {
	}
	return ch;
}

/* Overview:
 *  This function is used to write data at 'va' with length 'len' to a device physical address
 *  'pa'. Remember to check the validity of 'va' and 'pa' (see Hint below);
 *
 *  'va' is the starting address of source data, 'len' is the
 *  length of data (in bytes), 'pa' is the physical address of
 *  the device (maybe with a offset).
 *
 * Pre-Condition:
 *  'len' must be 1, 2 or 4, otherwise return -E_INVAL.
 *
 * Post-Condition:
 *  Data within [va, va+len) is copied to the physical address 'pa'.
 *  Return 0 on success.
 *  Return -E_INVAL on bad address.
 *
 * Hint:
 *  You can use 'is_illegal_va_range' to validate 'va'.
 *  You may use the unmapped and uncached segment in kernel address space (KSEG1)
 *  to perform MMIO by assigning a corresponding-lengthed data to the address,
 *  or you can just simply use the io function defined in 'include/io.h',
 *  such as 'iowrite32', 'iowrite16' and 'iowrite8'.
 *
 *  All valid device and their physical address ranges:
 *	* ---------------------------------*
 *	|   device   | start addr | length |
 *	* -----------+------------+--------*
 *	|  console   | 0x180003f8 | 0x20   |
 *	|  IDE disk  | 0x180001f0 | 0x8    |
 *	* ---------------------------------*
 */
int sys_write_dev(u_int va, u_int pa, u_int len) {
	/* Exercise 5.1: Your code here. (1/2) */
	// 判断内存的虚拟地址是否处于用户空间以及设备的物理地址是否处于设备的范围内
	if (is_illegal_va_range(va, len)) {
		return -E_INVAL;
	}

	if (len != 1 && len != 2 && len != 4) {
		return -E_INVAL;
	}

	if ((0x180003f8 <= pa && pa + len <= 0x18000418) ||
	    (0x180001f0 <= pa && pa + len <= 0x180001f8)) {
		// 调用 memcpy 从内存向设备写入
		memcpy((void *)(KSEG1 | pa), (void *)va, len);
		return 0;
	}

	return -E_INVAL;
}

/* Overview:
 *  This function is used to read data from a device physical address.
 *
 * Pre-Condition:
 *  'len' must be 1, 2 or 4, otherwise return -E_INVAL.
 *
 * Post-Condition:
 *  Data at 'pa' is copied from device to [va, va+len).
 *  Return 0 on success.
 *  Return -E_INVAL on bad address.
 *
 * Hint:
 *  You can use 'is_illegal_va_range' to validate 'va'.
 *  You can use function 'ioread32', 'ioread16' and 'ioread8' to read data from device.
 */
int sys_read_dev(u_int va, u_int pa, u_int len) {
	/* Exercise 5.1: Your code here. (2/2) */
	// 判断内存的虚拟地址是否处于用户空间以及设备的物理地址是否处于设备的范围内
	if (is_illegal_va_range(va, len)) {
		return -E_INVAL;
	}

	if (len != 1 && len != 2 && len != 4) {
		return -E_INVAL;
	}

	if ((0x180003f8 <= pa && pa + len <= 0x18000418) ||
	    (0x180001f0 <= pa && pa + len <= 0x180001f8)) {
		// 调用 memcpy 从设备读入内存
		memcpy((void *)va, (void *)(KSEG1 | pa), len);
		return 0;
	}

	return -E_INVAL;
}

/* 系统调用向量表 */
void *syscall_table[MAX_SYSNO] = {
    [SYS_putchar] = sys_putchar,
    [SYS_print_cons] = sys_print_cons,
    [SYS_getenvid] = sys_getenvid,
    [SYS_yield] = sys_yield,
    [SYS_env_destroy] = sys_env_destroy,
    [SYS_set_tlb_mod_entry] = sys_set_tlb_mod_entry,
    [SYS_mem_alloc] = sys_mem_alloc,
    [SYS_mem_map] = sys_mem_map,
    [SYS_mem_unmap] = sys_mem_unmap,
    [SYS_exofork] = sys_exofork,
    [SYS_set_env_status] = sys_set_env_status,
    [SYS_set_trapframe] = sys_set_trapframe,
    [SYS_panic] = sys_panic,
    [SYS_ipc_try_send] = sys_ipc_try_send,
    [SYS_ipc_recv] = sys_ipc_recv,
    [SYS_cgetc] = sys_cgetc,
    [SYS_write_dev] = sys_write_dev,
    [SYS_read_dev] = sys_read_dev,
};

/* Overview:
 *   Call the function in 'syscall_table' indexed at 'sysno' with arguments from user context and
 * stack.
 *
 * Hint:
 *   Use sysno from $a0 to dispatch the syscall.
 *   The possible arguments are stored at $a1, $a2, $a3, [$sp + 16 bytes], [$sp + 20 bytes] in
 *   order.
 *   Number of arguments cannot exceed 5.
 */
/**
 * 系统调用分发函数
 * 调用系统调用向量表中对应索引的系统调用函数
 * 通过 sysno 获取对应的系统调用函数
 * 传入用户上下文和栈中参数
 * 注意参数数量不能超过 5 个
*/
void do_syscall(struct Trapframe *tf) {
	int (*func)(u_int, u_int, u_int, u_int, u_int);
	int sysno = tf->regs[4];
	if (sysno < 0 || sysno >= MAX_SYSNO) {
		tf->regs[2] = -E_NO_SYS;
		return;
	}

	/* Step 1: Add the EPC in 'tf' by a word (size of an instruction). */
	/* 陷入帧的 cp0_epc 字段增加 4 个字节 */
	/* 无论系统调用是否成功都需要跳过当前指令 */
	/* Exercise 4.2: Your code here. (1/4) */
	tf->cp0_epc += 4;

	/* Step 2: Use 'sysno' to get 'func' from 'syscall_table'. */
	/* 使用 sysno 从系统调用向量表中获取对应的系统调用函数 */
	/* Exercise 4.2: Your code here. (2/4) */
	func = syscall_table[sysno];

	/* Step 3: First 3 args are stored in $a1, $a2, $a3. */
	/* 前面 3 个参数保存在寄存器中 */
	u_int arg1 = tf->regs[5];
	u_int arg2 = tf->regs[6];
	u_int arg3 = tf->regs[7];

	/* Step 4: Last 2 args are stored in stack at [$sp + 16 bytes], [$sp + 20 bytes]. */
	u_int arg4, arg5;
	/* 后面两个参数保存在栈中 */
	/* Exercise 4.2: Your code here. (3/4) */
	arg4 = *((u_int *)(tf->regs[29] + 16));
	arg5 = *((u_int *)(tf->regs[29] + 20));

	/* Step 5: Invoke 'func' with retrieved arguments and store its return value to $v0 in 'tf'.
	 */
	/* 使用接收到的参数调用 func 并将返回值写入陷入帧的 $v0 寄存器中  */
	/* Exercise 4.2: Your code here. (4/4) */
	tf->regs[2] = func(arg1, arg2, arg3, arg4, arg5);
}
