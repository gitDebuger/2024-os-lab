#include <asm/cp0regdef.h>
#include <elf.h>
#include <env.h>
#include <mmu.h>
#include <pmap.h>
#include <printk.h>
#include <sched.h>

/* 所有进程控制块 */
struct Env envs[NENV] __attribute__((aligned(PAGE_SIZE))); // All environments

/* 当前进程控制块指针 */
/* 初始化为空指针 */
struct Env *curenv = NULL;	      // the current env
/* 空闲进程控制块链表 */
static struct Env_list env_free_list; // Free list

// Invariant: 'env' in 'env_sched_list' iff. 'env->env_status' is 'RUNNABLE'.
/* 可运行进程控制块链表 */
/* env_sched_list 中的进程控制块的状态为可运行 */
struct Env_sched_list env_sched_list; // Runnable list

/* 页目录基地址 */
static Pde *base_pgdir;

/* 用于管理地址空间标识符 ASID 的位图 */
/* 位图说明这是一个状压数组 */
/* 读写相关信息时需要通过位运算 */
/* 该位于用于跟踪哪些 ASID 已经被分配给进程 */
/* 哪些尚未被分配 */
/* 该位图最初没有任何 ASID 被使用 */
/* vector<bit>(NSID / 32) */
static uint32_t asid_bitmap[NASID / 32] = {0};

/* Overview:
 *  Allocate an unused ASID.
 *
 * Post-Condition:
 *   return 0 and set '*asid' to the allocated ASID on success.
 *   return -E_NO_FREE_ENV if no ASID is available.
 */
/**
 * 申请一个没有被使用的 ASID
 * 如果申请成功则返回 0 并将 *asid 设置为申请到的 ASID 值
 * 否则返回 -E_NO_FREE_ENV 错误码
*/
static int asid_alloc(u_int *asid) {
	/* 从 0 到 NASID - 1 循环 */
	/* 也就是说将找到的第一个未分配的 ASID 分配给新创建的进程 */
	for (u_int i = 0; i < NASID; ++i) {
		/* 高位表示在状压数组中的 index */
		int index = i >> 5;
		/* 低 5 位表示在元素中的具体位置 */
		int inner = i & 31;
		/* 如果当前 ASID 未使用 */
		if ((asid_bitmap[index] & (1 << inner)) == 0) {
			/* 将当前 ASID 的状态设置为已使用 */
			asid_bitmap[index] |= 1 << inner;
			/* 将 *asid 设置为当前 ASID 值 */
			*asid = i;
			/* 返回 0 表示申请成功 */
			return 0;
		}
	}
	/* 所有 ASID 都已经被使用则返回错误码 -E_NO_FREE_ENV */
	return -E_NO_FREE_ENV;
}

/* Overview:
 *  Free an ASID.
 *
 * Pre-Condition:
 *  The ASID is allocated by 'asid_alloc'.
 *
 * Post-Condition:
 *  The ASID is freed and may be allocated again later.
 */
/**
 * 释放当前 ASID 即将当前 ASID 标记为空闲
 * 当前 ASID 是使用 asid_alloc 函数申请到的
 * 当前 ASID 被标记为空闲后可能在后续程序中再次被申请
*/
static void asid_free(u_int i) {
	int index = i >> 5;
	int inner = i & 31;
	asid_bitmap[index] &= ~(1 << inner);
}

/* Overview:
 *   Map [va, va+size) of virtual address space to physical [pa, pa+size) in the 'pgdir'. Use
 *   permission bits 'perm | PTE_V' for the entries.
 *
 * Pre-Condition:
 *   'pa', 'va' and 'size' are aligned to 'PAGE_SIZE'.
 */
/**
 * 将 [va, va + size) 的虚拟地址空间映射到 [pa, pa + size) 的物理地址空间
 * 页表项权限设置为 perm | PTE_V
 * 前置条件
 * `pa` `va` `size` 均已经按照 PAGE_SIZE 对齐
*/
static void map_segment(Pde *pgdir, u_int asid, u_long pa, u_long va, u_int size, u_int perm) {

	/* 确保三个变量已经按照 PAGE_SIZE 对齐 */
	assert(pa % PAGE_SIZE == 0);
	assert(va % PAGE_SIZE == 0);
	assert(size % PAGE_SIZE == 0);

	/* Step 1: Map virtual address space to physical address space. */
	/* 逐页将虚拟地址空间映射到物理地址空间 */
	for (int i = 0; i < size; i += PAGE_SIZE) {
		/*
		 * Hint:
		 *  Map the virtual page 'va + i' to the physical page 'pa + i' using 'page_insert'.
		 *  Use 'pa2page' to get the 'struct Page *' of the physical address.
		 */
		/* Exercise 3.2: Your code here. */
		/* 使用 page_insert 函数完成映射 */
		page_insert(pgdir, asid, pa2page(pa + i), va + i, perm);
	}
}

/* Overview:
 *  This function is to make a unique ID for every env
 *
 * Pre-Condition:
 *  e should be valid
 *
 * Post-Condition:
 *  return e's envid on success
 */
/**
 * 为每个进程控制块生成唯一的 PID 数值
 * 也就是说随着系统的运行
 * PID 可能被耗尽
 * 参数 e 需要是有效的进程控制块指针
 * 返回得到的 PID 数值
*/
u_int mkenvid(struct Env *e) {
	static u_int i = 0;
	return ((++i) << (1 + LOG2NENV)) | (e - envs);
}

/* Overview:
 *   Convert an existing 'envid' to an 'struct Env *'.
 *   If 'envid' is 0, set '*penv = curenv', otherwise set '*penv = &envs[ENVX(envid)]'.
 *   In addition, if 'checkperm' is non-zero, the requested env must be either 'curenv' or its
 *   immediate child.
 *
 * Pre-Condition:
 *   'penv' points to a valid 'struct Env *'.
 *
 * Post-Condition:
 *   return 0 on success, and set '*penv' to the env.
 *   return -E_BAD_ENV on error (invalid 'envid' or 'checkperm' violated).
 */
/**
 * 将存在的 envid 转换为进程控制块结构体
 * 如果 envid 为 0 则将 *penv 设置为 curenv 当前进程控制块
 * 否则将 *penv 设置为 &envs[ENVX(envid)]
 * 除此之外
 * 如果 checkperm 不是 0 那么请求的进程控制块必须是当前进程控制块或其直接子进程控制块
*/
int envid2env(u_int envid, struct Env **penv, int checkperm) {
	struct Env *e;

	/* Step 1: Assign value to 'e' using 'envid'. */
	/* Hint:
	 *   If envid is zero, set 'penv' to 'curenv' and return 0.
	 *   You may want to use 'ENVX'.
	 */
	/**
	 * 使用 envid 安排 e 的值
	 * 如果 envid 为 0 则将 *penv 设置为 curenv 并返回 0
	 * 可能需要使用 ENVX
	*/
	/* Exercise 4.3: Your code here. (1/2) */
	if (envid == 0) {
		*penv = curenv;
		return 0;
	} else {
		e = &envs[ENVX(envid)];
	}

	if (e->env_status == ENV_FREE || e->env_id != envid) {
		return -E_BAD_ENV;
	}

	/* Step 2: Check when 'checkperm' is non-zero. */
	/* Hints:
	 *   Check whether the calling env has sufficient permissions to manipulate the
	 *   specified env, i.e. 'e' is either 'curenv' or its immediate child.
	 *   If violated, return '-E_BAD_ENV'.
	 */
	/**
	 * 检查 checkperm 是否非零
	 * 检查调用的进程控制块是否有足够的权限操作特殊的进程控制块
	 * 否则检查 e 是否为 curenv 或其直接子进程控制块
	 * 如果不满足条件则返回 -E_BAD_ENV
	*/
	/* Exercise 4.3: Your code here. (2/2) */
	if (checkperm != 0) {
		if (e->env_id != curenv->env_id && e->env_parent_id != curenv->env_id) {
			return -E_BAD_ENV;
		}
	}

	/* Step 3: Assign 'e' to '*penv'. */
	/* 将 e 赋值给 *penv */
	*penv = e;
	return 0;
}

/* Overview:
 *   Mark all environments in 'envs' as free and insert them into the 'env_free_list'.
 *   Insert in reverse order, so that the first call to 'env_alloc' returns 'envs[0]'.
 *
 * Hints:
 *   You may use these macro definitions below: 'LIST_INIT', 'TAILQ_INIT', 'LIST_INSERT_HEAD'
 */
/**
 * 将 envs 中的所有进程控制块设置为空闲并将它们插入 env_free_list 空闲链表中
 * 插入时采用倒序插入方式
 * 也就是倒序遍历数组插入链表
 * 这样第一次调用 env_alloc 时返回的是 envs[0] 对应的进程控制块
*/
void env_init(void) {
	int i;
	/* Step 1: Initialize 'env_free_list' with 'LIST_INIT' and 'env_sched_list' with
	 * 'TAILQ_INIT'. */
	/* Exercise 3.1: Your code here. (1/2) */
	/* 使用 LIST_INIT 和 TAILQ_INIT 宏初始化 env_free_list 链表和 env_sched_list 链表 */
	LIST_INIT(&env_free_list);
	TAILQ_INIT(&env_sched_list);

	/* Step 2: Traverse the elements of 'envs' array, set their status to 'ENV_FREE' and insert
	 * them into the 'env_free_list'. Make sure, after the insertion, the order of envs in the
	 * list should be the same as they are in the 'envs' array. */
	/**
	 * 遍历进程控制块数组并将控制块状态设置为 ENV_FREE 并将它们插入 env_free_list 空闲控制块链表
	 * 确保插入后链表中的控制块顺序和数组中的控制块顺序相同
	 * 也就是说需要倒序遍历数组执行插入操作
	*/
	/* Exercise 3.1: Your code here. (2/2) */
	for (i = NENV - 1; i >= 0; --i) {
		LIST_INSERT_HEAD(&env_free_list, &(envs[i]), env_link);
		envs[i].env_status = ENV_FREE;
	}

	/*
	 * We want to map 'UPAGES' and 'UENVS' to *every* user space with PTE_G permission (without
	 * PTE_D), then user programs can read (but cannot write) kernel data structures 'pages' and
	 * 'envs'.
	 *
	 * Here we first map them into the *template* page directory 'base_pgdir'.
	 * Later in 'env_setup_vm', we will copy them into each 'env_pgdir'.
	 */
	/**
	 * 我们想要将 UPAGES 和 UENVS 映射到每个用户空间使用 PTE_G 权限  无 PTE_D 权限
	 * 这样做的目的是使得用户程序也能够通过 UPAGES 和 UENVS 的用户地址空间获取 Page 和 Env 的信息
	 * 设置该页将 pages 和 envs 即所有页控制块和所有进程控制块的内存空间
	 * 分别映射到 UPAGES 和 UENVS 的空间中
	 * 后续在 env_setup_vm 中我们将它们复制到 env_pgdir 中
	*/
	/* 首先调用 page_alloc 申请一个页 */
	struct Page *p;
	panic_on(page_alloc(&p));
	p->pp_ref++;

	/* 该页即模板页目录 */
	/* 我们把它的地址存储到全局变量 base_pgdir 中 */
	base_pgdir = (Pde *)page2kva(p);
	/* 在指定的页目录中创建虚拟地址空间 [va, va+size) 到物理地址空间 [pa, pa+size) 的映射 */
	map_segment(base_pgdir, 0, PADDR(pages), UPAGES, ROUND(npage * sizeof(struct Page), PAGE_SIZE),
				PTE_G);
	map_segment(base_pgdir, 0, PADDR(envs), UENVS, ROUND(NENV * sizeof(struct Env), PAGE_SIZE),
				PTE_G);
}

/* Overview:
 *   Initialize the user address space for 'e'.
 */
/* 为进程控制块 e 初始化用户地址空间 */
static int env_setup_vm(struct Env *e) {
	/* Step 1:
	 *   Allocate a page for the page directory with 'page_alloc'.
	 *   Increase its 'pp_ref' and assign its kernel address to 'e->env_pgdir'.
	 *
	 * Hint:
	 *   You can get the kernel address of a specified physical page using 'page2kva'.
	 */
	/* 调用 page_alloc 为页目录申请一页 */
	/* 增加页面引用次数并将它的内核地址赋值给 e->envpgdir */
	struct Page *p;
	try(page_alloc(&p));
	/* Exercise 3.3: Your code here. */
	++(p->pp_ref);
	e->env_pgdir = (Pde *)page2kva(p);

	/* Step 2: Copy the template page directory 'base_pgdir' to 'e->env_pgdir'. */
	/* Hint:
	 *   As a result, the address space of all envs is identical in [UTOP, UVPT).
	 *   See include/mmu.h for layout.
	 */
	/* 将模板页目录 base_pgdir 赋值给 e->env_pgidr */
	/* 因此在 [UTOP, UVPT) 中所有进程控制块的地址空间是相同的 */
	/* 在 include/mmu.h 中可以看到内存布局 */
	memcpy(e->env_pgdir + PDX(UTOP), base_pgdir + PDX(UTOP),
	       sizeof(Pde) * (PDX(UVPT) - PDX(UTOP)));

	/* Step 3: Map its own page table at 'UVPT' with readonly permission.
	 * As a result, user programs can read its page table through 'UVPT' */
	/* 给它自己在 UVPT 的页表添加 PTE_V 权限 */
	e->env_pgdir[PDX(UVPT)] = PADDR(e->env_pgdir) | PTE_V;
	return 0;
}

/* Overview:
 *   Allocate and initialize a new env.
 *   On success, the new env is stored at '*new'.
 *
 * Pre-Condition:
 *   If the new env doesn't have parent, 'parent_id' should be zero.
 *   'env_init' has been called before this function.
 *
 * Post-Condition:
 *   return 0 on success, and basic fields of the new Env are set up.
 *   return < 0 on error, if no free env, no free asid, or 'env_setup_vm' failed.
 *
 * Hints:
 *   You may need to use these functions or macros:
 *     'LIST_FIRST', 'LIST_REMOVE', 'mkenvid', 'asid_alloc', 'env_setup_vm'
 *   Following fields of Env should be set up:
 *     'env_id', 'env_asid', 'env_parent_id', 'env_tf.regs[29]', 'env_tf.cp0_status',
 *     'env_user_tlb_mod_entry', 'env_runs'
 */
/**
 * 申请并初始化新的 env 进程控制块
 * 如果申请成功则将申请到的新的进程控制块存储在 *new 中
 * 如果新的进程控制块没有父进程控制块
 * 则 parent_id 应该被设置为零
 * 在该函数调用前 env_init 函数已经被调用
 * 如果调用成功则返回 0 并且设置新进程控制块的基本字段
 * 错误时返回小于零的错误码
 * 可能得错误有无空闲进程控制块  无空闲 ASID   或者 env_setup_vm 失败
 * 以下是需要被设置的进程控制块字段
 * env_id
 * env_asid
 * env_parent_id
 * env_tf.regs[29]
 * env_tf.cp0_status
 * env_user_tlb_mod_entry
 * env_runs
*/
int env_alloc(struct Env **new, u_int parent_id) {
	int r;
	struct Env *e;

	/* Step 1: Get a free Env from 'env_free_list' */
	/* 从 env_free_list 中获取空闲进程控制块 */
	/* Exercise 3.4: Your code here. (1/4) */
	if (LIST_EMPTY(&env_free_list)) {
		return -E_NO_FREE_ENV;
	}
	e = LIST_FIRST(&env_free_list);

	/* Step 2: Call a 'env_setup_vm' to initialize the user address space for this new Env. */
	/* 调用 env_setup_vm 函数为新进程控制块初始化用户地址空间 */
	/* Exercise 3.4: Your code here. (2/4) */
	if ((r = env_setup_vm(e)) != 0) {
		return r;
	}

	/* Step 3: Initialize these fields for the new Env with appropriate values:
	 *   'env_user_tlb_mod_entry' (lab4), 'env_runs' (lab6), 'env_id' (lab3), 'env_asid' (lab3),
	 *   'env_parent_id' (lab3)
	 *
	 * Hint:
	 *   Use 'asid_alloc' to allocate a free asid.
	 *   Use 'mkenvid' to allocate a free envid.
	 */
	/**
	 * 使用正确的值为新进程控制块初始化字段
	 * env_user_tlb_mod_entry (lab4)
	 * env_runs (lab6)
	 * env_id (lab3)
	 * env_asid (lab3)
	 * env_parent_id (lab3)
	 * 使用 asid_alloc 函数申请空闲 ASID
	 * 使用 mkenvid 函数申请空闲 PID
	*/
	e->env_user_tlb_mod_entry = 0; // for lab4
	e->env_runs = 0;	       // for lab6
	/* Exercise 3.4: Your code here. (3/4) */
	e->env_id = mkenvid(e);
	e->env_parent_id = parent_id;
	if ((r = asid_alloc(&(e->env_asid))) != 0) {
		return r;
	}

	/* Step 4: Initialize the sp and 'cp0_status' in 'e->env_tf'.
	 *   Set the EXL bit to ensure that the processor remains in kernel mode during context
	 * recovery. Additionally, set UM to 1 so that when ERET unsets EXL, the processor
	 * transitions to user mode.
	 */
	/**
	 * 初始化 e->env_tf 中的栈指针 sp 和 cp0_status 寄存器
	 * 设置 EXL 位确保处理器在上下文恢复期间保持在内核态
	 * 除此之外将 UM 设置为 1 以便当 ERET 取消设置 EXL 时处理器能够转换到用户态
	*/
	e->env_tf.cp0_status = STATUS_IM7 | STATUS_IE | STATUS_EXL | STATUS_UM;
	// 为 argc 和 argv 预留空间
	e->env_tf.regs[29] = USTACKTOP - sizeof(int) - sizeof(char **);

	/* Step 5: Remove the new Env from env_free_list. */
	/* 将新的进程控制块从 env_free_list 中移除 */
	/* Exercise 3.4: Your code here. (4/4) */
	LIST_REMOVE(e, env_link);

	/* 给 *new 赋值并返回 */
	*new = e;
	return 0;
}

/* Overview:
 *   Load a page into the user address space of an env with permission 'perm'.
 *   If 'src' is not NULL, copy the 'len' bytes from 'src' into 'offset' at this page.
 *
 * Pre-Condition:
 *   'offset + len' is not larger than 'PAGE_SIZE'.
 *
 * Hint:
 *   The address of env structure is passed through 'data' from 'elf_load_seg', where this function
 *   works as a callback.
 *
 * Note:
 *   This function involves loading executable code to memory. After the completion of load
 *   procedures, D-cache and I-cache writeback/invalidation MUST be performed to maintain cache
 *   coherence, which MOS has NOT implemented. This may result in unexpected behaviours on real
 *   CPUs! QEMU doesn't simulate caching, allowing the OS to function correctly.
 */
/**
 * 使用权限 perm 将一页加载到进程控制块的用户地址空间
 * 如果 src 非空则将 len 字节的数据从 src 拷贝到页面的 offset 位置
 * offset + len 不会比 PAGE_SIZE 更大
 * 进程控制块结构体的地址通过 data 从 elf_load_seg 传递
 * 该函数 load_icode_mapper 作为回调函数
 * 这个函数包括将可执行代码加载到内存中
 * 在加载过程完成后
 * 必须执行 D-cache 和 I-cache 回写/无效操作以保持缓存一致性
 * 这是MOS没有实现的
 * 这可能会导致在真实 cpu 上出现意想不到的行为
 * QEMU不模拟缓存
 * 允许操作系统正常工作。
*/
static int load_icode_mapper(void *data, u_long va, size_t offset, u_int perm, const void *src,
			     size_t len) {
	struct Env *env = (struct Env *)data;
	struct Page *p;
	int r;

	/* Step 1: Allocate a page with 'page_alloc'. */
	/* 使用 page_alloc 函数申请一个页面 */
	/* Exercise 3.5: Your code here. (1/2) */
	if ((r = page_alloc(&p)) != 0) {
		return r;
	}

	/* Step 2: If 'src' is not NULL, copy the 'len' bytes started at 'src' into 'offset' at this
	 * page. */
	// Hint: You may want to use 'memcpy'.
	/* 如果 src 非空则将 len 字节的数据从 src 拷贝到页面的 offset 位置 */
	if (src != NULL) {
		/* Exercise 3.5: Your code here. (2/2) */
		memcpy((void *)(page2kva(p) + offset), src, len);
	}

	/* Step 3: Insert 'p' into 'env->env_pgdir' at 'va' with 'perm'. */
	/* 将页面 p 插入到 env->env_pgdir 映射到虚拟地址 va 使用权限 perm */
	return page_insert(env->env_pgdir, env->env_asid, p, va, perm);
}

/* Overview:
 *   Load program segments from 'binary' into user space of the env 'e'.
 *   'binary' points to an ELF executable image of 'size' bytes, which contains both text and data
 *   segments.
 */
/**
 * 将程序从 binary 加载到进程控制块 e 的用户空间
 * binary 指向一个大小为 size 字节的 ELF 可执行映像
 * 其中包含文本和数据段
*/
static void load_icode(struct Env *e, const void *binary, size_t size) {
	/* Step 1: Use 'elf_from' to parse an ELF header from 'binary'. */
	/* 使用 elf_from 从 binary 解析一个 ELF 头 */
	const Elf32_Ehdr *ehdr = elf_from(binary, size);
	if (!ehdr) {
		panic("bad elf at %x", binary);
	}

	/* Step 2: Load the segments using 'ELF_FOREACH_PHDR_OFF' and 'elf_load_seg'.
	 * As a loader, we just care about loadable segments, so parse only program headers here.
	 */
	/* 使用 ELF_FOREACH_PHDR_OFF 和 elf_load_seg 加载段 */
	/* 作为加载者 */
	/* 我们只在乎可加载段 */
	/* 所以这里只解析程序头 */
	size_t ph_off;
	/* 遍历所有的程序头表 */
	ELF_FOREACH_PHDR_OFF (ph_off, ehdr) {
		Elf32_Phdr *ph = (Elf32_Phdr *)(binary + ph_off);
		/* 在循环中取出对应的程序头 */
		/* 如果其中的 p_type 类型为 PT_LOAD */
		/* 说明其对应的程序需要被加载到内存中 */
		if (ph->p_type == PT_LOAD) {
			// 'elf_load_seg' is defined in lib/elfloader.c
			// 'load_icode_mapper' defines the way in which a page in this segment
			// should be mapped.
			/* 调用 elf_load_seg 函数来进行加载 */
			panic_on(elf_load_seg(ph, binary + ph->p_offset, load_icode_mapper, e));
		}
	}

	/* Step 3: Set 'e->env_tf.cp0_epc' to 'ehdr->e_entry'. */
	/* 将进程控制块中 env_tf 的 cp0_epc 寄存器的值设置为 ELF 文件中设定的程序入口地址 */
	/* Exercise 3.6: Your code here. */
	e->env_tf.cp0_epc = ehdr->e_entry;

}

/* Overview:
 *   Create a new env with specified 'binary' and 'priority'.
 *   This is only used to create early envs from kernel during initialization, before the
 *   first created env is scheduled.
 *
 * Hint:
 *   'binary' is an ELF executable image in memory.
 */
/* 使用 binary 和 priority 参数创建一个新的进程控制块 */
/* 这只用于在初始化期间从内核创建早期进程 */
/* 在第一个创建的进程被调度之前 */
/* binary 是内存中一个 ELF 可执行文件映像 */
struct Env *env_create(const void *binary, size_t size, int priority) {
	struct Env *e;
	/* Step 1: Use 'env_alloc' to alloc a new env, with 0 as 'parent_id'. */
	/* 调用 env_alloc 申请一个新的进程控制块 */
	/* 使用 0 作为父进程的 PID */
	/* Exercise 3.7: Your code here. (1/3) */
	env_alloc(&e, 0);

	/* Step 2: Assign the 'priority' to 'e' and mark its 'env_status' as runnable. */
	/* Exercise 3.7: Your code here. (2/3) */
	/* 设置 进程控制块 e 的优先级并将其状态标记为可运行 */
	e->env_pri = priority;
	e->env_status = ENV_RUNNABLE;

	/* Step 3: Use 'load_icode' to load the image from 'binary', and insert 'e' into
	 * 'env_sched_list' using 'TAILQ_INSERT_HEAD'. */
	/* 使用 load_icode 从 binary 中加载映像 */
	/* 并使用 TAILQ_INSERT_HEAD 将 e 插入到 env_sched_list 中 */
	/* Exercise 3.7: Your code here. (3/3) */
	load_icode(e, binary, size);
	TAILQ_INSERT_HEAD(&env_sched_list, e, env_sched_link);

	return e;
}

/* Overview:
 *  Free env e and all memory it uses.
 */
/* 释放进程控制块 e 以及所有它使用的内存 */
void env_free(struct Env *e) {
	Pte *pt;
	u_int pdeno, pteno, pa;

	/* Hint: Note the environment's demise.*/
	/* 请注意环境的消亡 */
	printk("[%08x] free env %08x\n", curenv ? curenv->env_id : 0, e->env_id);

	/* Hint: Flush all mapped pages in the user portion of the address space */
	/* 刷新地址空间的用户部分中的所有映射页 */
	for (pdeno = 0; pdeno < PDX(UTOP); pdeno++) {
		/* Hint: only look at mapped page tables. */
		/* 只看映射的页表 */
		if (!(e->env_pgdir[pdeno] & PTE_V)) {
			continue;
		}
		/* Hint: find the pa and va of the page table. */
		/* 寻找页表中的 pa 和 va 内容 */
		pa = PTE_ADDR(e->env_pgdir[pdeno]);
		pt = (Pte *)KADDR(pa);
		/* Hint: Unmap all PTEs in this page table. */
		/* 取消页表中所有 PTE 的映射 */
		for (pteno = 0; pteno <= PTX(~0); pteno++) {
			if (pt[pteno] & PTE_V) {
				page_remove(e->env_pgdir, e->env_asid,
					    (pdeno << PDSHIFT) | (pteno << PGSHIFT));
			}
		}
		/* Hint: free the page table itself. */
		/* 释放页表本身 */
		e->env_pgdir[pdeno] = 0;
		page_decref(pa2page(pa));
		/* Hint: invalidate page table in TLB */
		/* 使 TLB 中的页表无效化 */
		tlb_invalidate(e->env_asid, UVPT + (pdeno << PGSHIFT));
	}
	/* Hint: free the page directory. */
	/* 释放页目录 */
	page_decref(pa2page(PADDR(e->env_pgdir)));
	/* Hint: free the ASID */
	/* 释放 ASID */
	asid_free(e->env_asid);
	/* Hint: invalidate page directory in TLB */
	/* 使 TLB 中的页目录无效化 */
	tlb_invalidate(e->env_asid, UVPT + (PDX(UVPT) << PGSHIFT));
	/* Hint: return the environment to the free list. */
	/* 将进程控制块归还给空闲链表 */
	e->env_status = ENV_FREE;
	LIST_INSERT_HEAD((&env_free_list), (e), env_link);
	TAILQ_REMOVE(&env_sched_list, (e), env_sched_link);
}

/* Overview:
 *  Free env e, and schedule to run a new env if e is the current env.
 */
/* 释放进程控制块 e 然后调度一个新的进程如果 e 是当前进程控制块 */
void env_destroy(struct Env *e) {
	/* Hint: free e. */
	/* 释放 e */
	env_free(e);

	/* Hint: schedule to run a new environment. */
	/* 调度一个新的进程控制块执行 */
	if (curenv == e) {
		curenv = NULL;
		printk("i am killed ... \n");
		schedule(1);
	}
}

// WARNING BEGIN: DO NOT MODIFY FOLLOWING LINES!
#ifdef MOS_PRE_ENV_RUN
#include <generated/pre_env_run.h>
#endif
// WARNING END

/* 定义在 kern/env_asm.S 中 */
extern void env_pop_tf(struct Trapframe *tf, u_int asid) __attribute__((noreturn));

/* Overview:
 *   Switch CPU context to the specified env 'e'.
 *
 * Post-Condition:
 *   Set 'e' as the current running env 'curenv'.
 *
 * Hints:
 *   You may use these functions: 'env_pop_tf'.
 */
/* 将 CPU 上下文切换到进程控制块 e */
/* 设置 e 作为当前进程控制块 curenv */
void env_run(struct Env *e) {
	assert(e->env_status == ENV_RUNNABLE);
	// WARNING BEGIN: DO NOT MODIFY FOLLOWING LINES!
#ifdef MOS_PRE_ENV_RUN
	MOS_PRE_ENV_RUN_STMT
#endif
	// WARNING END

	/* Step 1:
	 *   If 'curenv' is NULL, this is the first time through.
	 *   If not, we may be switching from a previous env, so save its context into
	 *   'curenv->env_tf' first.
	 */
	/* 如果 curenv 为空那么这是第一次调度 */
	/* 如果不为空我们可能需要从之前的进程转换过来 */
	/* 所以需要保存之前进程的上下文信息 */
	/* 具体操作为将栈帧信息保存在 curenv->env_tf 中 */
	if (curenv) {
		curenv->env_tf = *((struct Trapframe *)KSTACKTOP - 1);
	}

	/* Step 2: Change 'curenv' to 'e'. */
	/* 将当前进程控制块设置为 e */
	curenv = e;
	curenv->env_runs++; // lab6

	/* Step 3: Change 'cur_pgdir' to 'curenv->env_pgdir', switching to its address space. */
	/* Exercise 3.8: Your code here. (1/2) */
	/* 将 cur_pgdir 更新为 curenv->env_pgdir 以转换到新的地址空间 */
	cur_pgdir = curenv->env_pgdir;

	/* Step 4: Use 'env_pop_tf' to restore the curenv's saved context (registers) and return/go
	 * to user mode.
	 *
	 * Hint:
	 *  - You should use 'curenv->env_asid' here.
	 *  - 'env_pop_tf' is a 'noreturn' function: it restores PC from 'cp0_epc' thus not
	 *    returning to the kernel caller, making 'env_run' a 'noreturn' function as well.
	 */
	/* Exercise 3.8: Your code here. (2/2) */
	/* 使用 env_pop_tf 函数存储当前进程的上下文信息然后回到用户态 */
	/* 这里应该使用 curenv->env_asid 参数 */
	/* env_pop_tf 是一个 noreturn 函数 */
	/* 它从 cp0_epc 恢复 PC 寄存器从而不返回到内核调用者 */
	/* 使 env_run 也成为一个 noreturn 函数 */
	env_pop_tf(&curenv->env_tf, curenv->env_asid);

}

void env_check() {
	struct Env *pe, *pe0, *pe1, *pe2;
	struct Env_list fl;
	u_long page_addr;
	/* should be able to allocate three envs */
	pe0 = 0;
	pe1 = 0;
	pe2 = 0;
	assert(env_alloc(&pe0, 0) == 0);
	assert(env_alloc(&pe1, 0) == 0);
	assert(env_alloc(&pe2, 0) == 0);

	assert(pe0);
	assert(pe1 && pe1 != pe0);
	assert(pe2 && pe2 != pe1 && pe2 != pe0);

	/* temporarily steal the rest of the free envs */
	fl = env_free_list;
	/* now this env_free list must be empty! */
	LIST_INIT(&env_free_list);

	/* should be no free memory */
	assert(env_alloc(&pe, 0) == -E_NO_FREE_ENV);

	/* recover env_free_list */
	env_free_list = fl;

	printk("pe0->env_id %d\n", pe0->env_id);
	printk("pe1->env_id %d\n", pe1->env_id);
	printk("pe2->env_id %d\n", pe2->env_id);

	assert(pe0->env_id == 2048);
	assert(pe1->env_id == 4097);
	assert(pe2->env_id == 6146);
	printk("env_init() work well!\n");

	/* 'UENVS' and 'UPAGES' should have been correctly mapped in *template* page directory
	 * 'base_pgdir'. */
	for (page_addr = 0; page_addr < npage * sizeof(struct Page); page_addr += PAGE_SIZE) {
		assert(va2pa(base_pgdir, UPAGES + page_addr) == PADDR(pages) + page_addr);
	}
	for (page_addr = 0; page_addr < NENV * sizeof(struct Env); page_addr += PAGE_SIZE) {
		assert(va2pa(base_pgdir, UENVS + page_addr) == PADDR(envs) + page_addr);
	}
	/* check env_setup_vm() work well */
	printk("pe1->env_pgdir %x\n", pe1->env_pgdir);

	assert(pe2->env_pgdir[PDX(UTOP)] == base_pgdir[PDX(UTOP)]);
	assert(pe2->env_pgdir[PDX(UTOP) - 1] == 0);
	printk("env_setup_vm passed!\n");

	printk("pe2`s sp register %x\n", pe2->env_tf.regs[29]);

	/* free all env allocated in this function */
	TAILQ_INSERT_TAIL(&env_sched_list, pe0, env_sched_link);
	TAILQ_INSERT_TAIL(&env_sched_list, pe1, env_sched_link);
	TAILQ_INSERT_TAIL(&env_sched_list, pe2, env_sched_link);

	env_free(pe2);
	env_free(pe1);
	env_free(pe0);

	printk("env_check() succeeded!\n");
}

void envid2env_check() {
	struct Env *pe, *pe0, *pe2;
	assert(env_alloc(&pe0, 0) == 0);
	assert(env_alloc(&pe2, 0) == 0);
	int re;
	pe2->env_status = ENV_FREE;
	re = envid2env(pe2->env_id, &pe, 0);

	assert(re == -E_BAD_ENV);

	pe2->env_status = ENV_RUNNABLE;
	re = envid2env(pe2->env_id, &pe, 0);

	assert(pe->env_id == pe2->env_id && re == 0);

	curenv = pe0;
	re = envid2env(pe2->env_id, &pe, 1);
	assert(re == -E_BAD_ENV);
	printk("envid2env() work well!\n");
}
