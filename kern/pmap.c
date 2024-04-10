#include <bitops.h>
#include <env.h>
#include <malta.h>
#include <mmu.h>
#include <pmap.h>
#include <printk.h>
#include <buddy.h>

/* These variables are set by mips_detect_memory(ram_low_size); */
/* 通过mips_detect_memory(ram_low_size)函数设置 */
/* 最大物理可用内存 */
static u_long memsize; /* Maximum physical address */
/* 可用内存页面数 */
u_long npage;	       /* Amount of memory(in pages) */

/* typedef u_long Pde */
/* Pde -> Page directory entry */
/* 当前页目录指针 */
Pde *cur_pgdir;

/*
#define LIST_ENTRY(type) \
	struct { \
		struct type *le_next; \
		struct type **le_prev; \
	}
typedef LIST_ENTRY(Page) Page_LIST_entry_t;
struct Page {
	Page_LIST_entry_t pp_link;
	u_short pp_ref;
};
完整的结构体定义为
struct Page {
	struct {
		struct Page *le_next;
		struct Page **le_prev;
	} pp_link;
	u_short pp_ref;
};
*/

/* 页面链表节点结构体指针 */
struct Page *pages;
/* 空闲内存空间大小 */
static u_long freemem;

/*
#define LIST_HEAD(name, type) \
	struct name { \
		struct type *lh_first; \
	}
LIST_HEAD(Page_list, Page);
这个宏定义了这样的一个结构体
struct Page_list {
	struct Page *lh_first;
}
展开后是这样的内容
struct Page_list {
	struct {
		struct {
			struct Page *le_next;
			struct Page **le_prev;
		} pplink;
		u_short pp_ref;
	} *lh_first;
}
*/

/* 空闲物理页面链表 */
struct Page_list page_free_list; /* Free list of physical pages */

/* Overview:
 *   Use '_memsize' from bootloader to initialize 'memsize' and
 *   calculate the corresponding 'npage' value.
 */
/* 利用传入的内存空间大小信息初始化memsize变量和npage变量 */
void mips_detect_memory(u_int _memsize) {
	/* Step 1: Initialize memsize. */
	memsize = _memsize;

	/* Step 2: Calculate the corresponding 'npage' value. */
	/* Exercise 2.1: Your code here. */
	npage = memsize / PAGE_SIZE;
	printk("Memory size: %lu KiB, number of pages: %lu\n", memsize / 1024, npage);
}

/* Lab 2 Key Code "alloc" */
/* Overview:
 * Allocate `n` bytes physical memory with alignment `align`, if `clear` is set, 
 * clear the allocated memory.
 * This allocator is used only while setting up virtual memory system.
 * Post-Condition:
 * If we're out of memory, should panic, else return this address of memory we have allocated.
 */
/*
 * 分配 `n` 字节的物理内存使用 `align` 对齐方式
 * 如果设置 `clear` 则清空已分配的内存
 * 此分配器仅在设置虚拟内存系统时使用
 * 也就是仅在建立页式虚拟内存管理机制之前使用
 * 后置条件：
 * 如果内存不足应该触发 `panic` 否则返回分配的内存地址
 */
/**
 * @param n 要分配的内存空间大小
 * @param align 对齐方式——按照align字节对齐
 * @param clear 是否清空内存
*/
void *alloc(u_int n, u_int align, int clear) {
	/* 外部end变量 */
	/* 在kernel.lds中有定义 */
	extern char end[];
	/* 保存分配的内存地址 */
	u_long alloced_mem;

	/* Initialize `freemem` if this is the first time. The first virtual address that the
	 * linker did *not* assign to any kernel code or global variables. */
	/* freemem定义为全局变量且已经初始化为0 */
	/* 如果值为0说明是第一次分配内存 */
	/* 则需要设置为内存结束地址end */
	/* 也就是链接器没有分配给任何内核代码或者全局变量的起始虚拟内存地址 */
	if (freemem == 0) {
		freemem = (u_long)end;
	}

	/* Step 1: Round up `freemem` up to be aligned properly */
	/* 将freemem向上舍入到与align对齐的地址以确保内存分配满足对齐要求。 */
	freemem = ROUND(freemem, align);

	/* Step 2: Save current value of `freemem` as allocated chunk. */
	/* 将当前的freemem值保存到alloced_mem中以便后续返回分配的内存地址。 */
	alloced_mem = freemem;

	/* Step 3: Increase `freemem` to record allocation. */
	/* 将freemem增加n以记录已分配的内存。 */
	freemem = freemem + n;

	/* Panic if we're out of memory. */
	/* 如果已经超出了可用内存大小memsize则触发 panic。 */
	panic_on(PADDR(freemem) >= memsize);

	/* Step 4: Clear allocated chunk if parameter `clear` is set. */
	/* 如果需要清空内存就使用memset函数将分配的内存清零。 */
	if (clear) {
		memset((void *)alloced_mem, 0, n);
	}

	/* Step 5: return allocated chunk. */
	/* 返回已分配的内存地址 */
	return (void *)alloced_mem;
}
/**
 * low ------------------------------------------------> high
 * [] [][][][][] [][][][][] [][][][][][][][][][] [][][][][][]
 *   ^          ^          ^                    ^
 *  end      freemem   ROUND(freemem, align)  freemem += n
 *                                 └─> allocaed_mem
*/
/* 该代码操作kseg0段虚拟内存 */
/* 通过Lab2的内存管理工作使将来kuseg段虚拟内存能够正常使用 */
/* End of Key Code "alloc" */

/** Overview:
 * Set up two-level page table.
 * Hint:
 * You can get more details about `UPAGES` and `UENVS` 
 * in include/mmu.h.
*/
/* 建立两级页表管理机制 */
void mips_vm_init() {
	/* Allocate proper size of physical memory for global array `pages`,
	 * for physical memory management. Then, map virtual address `UPAGES` to
	 * physical address `pages` allocated before. For consideration of alignment,
	 * you should round up the memory size before map. */
	/**
	 * 首先申请合适大小的物理内存空间分配给全局数组pages用于物理内存管理
	 * 然后将虚拟地址UPAGES映射到刚才申请的物理地址pages上
	 * 考虑到内存对齐需要在映射前将内存空间大小向上对齐
	 * 也就是说按页面大小PAGE_SIZE对齐申请内存空间用于存储Pages结构体数组
	*/
	pages = (struct Page *)alloc(npage * sizeof(struct Page), PAGE_SIZE, 1);
	printk("to memory %x for struct Pages.\n", freemem);
	printk("pmap.c:\t mips vm init success\n");
}

/* Overview:
 *   Initialize page structure and memory free list. The 'pages' array has one 'struct Page' entry
 * per physical page. Pages are reference counted, and free pages are kept on a linked list.
 *
 * Hint: Use 'LIST_INSERT_HEAD' to insert free pages to 'page_free_list'.
 */
/**
 * 初始化页面结构体和空闲内存链表
 * 每个物理页框都对应一个Page结构体
 * 页面结构体中包含引用计数pp_ref成员变量
 * 所有的空闲页面保存在一个链表中
*/
void page_init(void) {
	/* Step 1: Initialize page_free_list. */
	/* Hint: Use macro `LIST_INIT` defined in include/queue.h. */
	/* Exercise 2.3: Your code here. (1/4) */
	/* 初始化空闲页面链表 */
	LIST_INIT(&page_free_list);

	/* Step 2: Align `freemem` up to multiple of PAGE_SIZE. */
	/* Exercise 2.3: Your code here. (2/4) */
	/* 将空闲内存地址按照页面大小向上对齐 */
	freemem = ROUND(freemem, PAGE_SIZE);

	/* Step 3: Mark all memory below `freemem` as used (set `pp_ref` to 1) */
	/* Exercise 2.3: Your code here. (3/4) */
	/* 将低于freemem地址的页面标记为已使用 */
	/* 通过将引用计数设置为1完成 */

	/* freemem是第一个空闲页面的起始地址 */
	/* PADDR(freemem)返回该物理地址对应的物理页框号 */
	/* 也就是说物理页框号小于count的物理页框都已经被使用 */
	u_long count = PPN(PADDR(freemem));
	/* 循环将引用设置为1标记为已使用 */
	for (u_long i = 0; i < count; ++i) {
		pages[i].pp_ref = 1;
	}

	/* Step 4: Mark the other memory as free. */
	/* Exercise 2.3: Your code here. (4/4) */
	/* 标记其他内存空间为未使用 */

	/* 从第一个空闲页面开始到到最后一个页面 */
	/* 循环将页面引用计数设置为0表示未使用 */
	/* 然后将这些空闲页面插入到空闲页面链表中 */
	for (u_long i = count; i < npage; ++i) {
		pages[i].pp_ref = 0;
		LIST_INSERT_HEAD(&page_free_list, &pages[i], pp_link);
	}
}

/* Overview:
 *   Allocate a physical page from free memory, and fill this page with zero.
 *
 * Post-Condition:
 *   If failed to allocate a new page (out of memory, there's no free page), return -E_NO_MEM.
 *   Otherwise, set the address of the allocated 'Page' to *pp, and return 0.
 *
 * Note:
 *   This does NOT increase the reference count 'pp_ref' of the page - the caller must do these if
 *   necessary (either explicitly or via page_insert).
 *
 * Hint: Use LIST_FIRST and LIST_REMOVE defined in include/queue.h.
 */
/**
 * 从空闲内存中申请一个物理页面然后用0填充
 * 后置条件：
 * 如果申请失败则返回-E_NO_MEM
 * 否则将*new设置为新申请的页面并返回0
 * 注意：
 * 这一步操作并不增加页面的引用计数——调用者必须这样做如果有必要的话
 * 或者直接操作引用计数或者通过page_insert函数
*/
/* page_free_list -> page_1 -> page_2 -> ... -> page_m */
/* 这是一个特殊的双向链表 */
int page_alloc(struct Page **new) {
	/* Step 1: Get a page from free memory. If fails, return the error code.*/
	struct Page *pp;
	/* Exercise 2.4: Your code here. (1/2) */
	/* 判定是否存在空闲页面 */
	if (LIST_EMPTY(&page_free_list)) {
		return -E_NO_MEM;
	}
	/* 获取空闲页面链表中的第一个页面 */
	pp = LIST_FIRST(&page_free_list);

	/* 从空闲链表中移除这一页面 */
	LIST_REMOVE(pp, pp_link);

	/* Step 2: Initialize this page with zero.
	 * Hint: use `memset`. */
	/* Exercise 2.4: Your code here. (2/2) */
	/* 将对应内存地址的数据清零 */
	/* page2kva将传入的页面结构体指针转换为内核虚拟地址 */
	/* page to kernel virtual address */
	memset((void*)page2kva(pp), 0, PAGE_SIZE);

	*new = pp;
	return 0;
}

/* Overview:
 *   Release a page 'pp', mark it as free.
 *
 * Pre-Condition:
 *   'pp->pp_ref' is '0'.
 */
/* 回收页面并将其插入空闲页面链表中 */
/* 在回收之前首先判断引用计数是否为零 */
void page_free(struct Page *pp) {
	assert(pp->pp_ref == 0);
	/* Just insert it into 'page_free_list'. */
	/* Exercise 2.5: Your code here. */
	LIST_INSERT_HEAD(&page_free_list, pp, pp_link);

}

/* Overview:
 *   Given 'pgdir', a pointer to a page directory, 'pgdir_walk' returns a pointer to
 *   the page table entry for virtual address 'va'.
 *
 * Pre-Condition:
 *   'pgdir' is a two-level page table structure.
 *   'ppte' is a valid pointer, i.e., it should NOT be NULL.
 *
 * Post-Condition:
 *   If we're out of memory, return -E_NO_MEM.
 *   Otherwise, we get the page table entry, store
 *   the value of page table entry to *ppte, and return 0, indicating success.
 *
 * Hint:
 *   We use a two-level pointer to store page table entry and return a state code to indicate
 *   whether this function succeeds or not.
 */
/**
 * @param pgdir 一级页表基地址
 * @param va 虚拟地址
 * @param create 创建位
 * @param ppte 虚拟地址va所在的页表项指针存放位置
 * @return 函数执行结果——失败返回-E_NO_MEM错误码
*/
static int pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte) {
	Pde *pgdir_entryp;
	struct Page *pp;

	/* Step 1: Get the corresponding page directory entry. */
	/* Exercise 2.6: Your code here. (1/3) */
	/* 页目录项地址等于页目录基地址加上虚拟地址va对应的偏移量 */
	pgdir_entryp = pgdir + PDX(va);
	/* 获取页目录项的值 */
	Pde pgdir_entry_data = *pgdir_entryp;

	/* Step 2: If the corresponding page table is not existent (valid) then:
	 *   * If parameter `create` is set, create one. Set the permission bits 'PTE_C_CACHEABLE |
	 *     PTE_V' for this new page in the page directory. If failed to allocate a new page (out
	 *     of memory), return the error.
	 *   * Otherwise, assign NULL to '*ppte' and return 0.
	 */
	/* Exercise 2.6: Your code here. (2/3) */
	/* Get low 12 bit of the page directory entry */
	// u_long perm = pgdir_entry_data & 0xfff;
	/* Why using *pgdir_entryp instead of perm? */
	/* 获取有效位的信息 */
	/* 如果对应页目录项无效 */
	if ((pgdir_entry_data & PTE_V) == 0) {
		/* 如果允许创建 */
		if (create) {
			/* 调用page_alloc函数申请新的页面到pp中用于存储新的页表 */
			int return_code = page_alloc(&pp);
			/* 申请失败则返回错误码 */
			if (return_code == -E_NO_MEM) {
				return -E_NO_MEM;
			}
			/* Why increase pp_ref instead of setting pp_ref to 1? */
			/* 增加引用计数 */
			++(pp->pp_ref);
			/* Why? */
			/* 将新申请的页面的物理地址以及权限位写入到页目录项中 */
			/* page2pa(pp)代表物理页表基地址？ */
			*pgdir_entryp = page2pa(pp) | (PTE_C_CACHEABLE | PTE_V);
			pgdir_entry_data = *pgdir_entryp;
		} else {
			/* 如果不允许创建则将*ppte设置为NULL */
			*ppte = NULL;
			return 0;
		}
	}

	/* Step 3: Assign the kernel virtual address of the page table entry to '*ppte'. */
	/* Exercise 2.6: Your code here. (3/3) */
	/* Why? */
	/* 无论是不是新创建的页表都进入这一步骤 */
	/* PTE_ADDR(pgdir_entry_data)从页目录项中解析出页表基地址 */
	/* 再调用KADDR将页表基地址转换为内核虚拟地址 */
	/* 然后强制类型转换赋值给pgtable作为页表基地址 */
	Pte *pgtable = (Pte *)KADDR(PTE_ADDR(pgdir_entry_data));
	/* 页表基地址加上虚拟地址va对应的偏移量 */
	*ppte = pgtable + PTX(va);

	return 0;
}

/* Overview:
 *   Map the physical page 'pp' at virtual address 'va'. The permission (the low 12 bits) of the
 *   page table entry should be set to 'perm | PTE_C_CACHEABLE | PTE_V'.
 *
 * Post-Condition:
 *   Return 0 on success
 *   Return -E_NO_MEM, if page table couldn't be allocated
 *
 * Hint:
 *   If there is already a page mapped at `va`, call page_remove() to release this mapping.
 *   The `pp_ref` should be incremented if the insertion succeeds.
 */
/**
 * 将物理页面pp映射到虚拟地址va并设置页表项权限
 * @param pgdir 页目录基地址
 * @param asid 操作TLB时使用
 * @param pp 页面
 * @param va 虚拟地址
 * @param perm 需要设置的权限
 * @return 函数执行状态码
*/
int page_insert(Pde *pgdir, u_int asid, struct Page *pp, u_long va, u_int perm) {
	Pte *pte;

	/* Step 1: Get corresponding page table entry. */
	/* 调用函数获取二级页表项 */
	/* 这里create参数为0不允许创建新的页表 */
	pgdir_walk(pgdir, va, 0, &pte);

	/* 如果pte不为空并且pte有效说明存在映射 */
	if (pte && (*pte & PTE_V)) {
		/* 如果当前映射页面和要插入的页不相同 */
		if (pa2page(*pte) != pp) {
			/* 则先移除现有的映射关系 */
			page_remove(pgdir, asid, va);
		} else {
			/* 否则更改权限即可 */
			/* 设置TLB无效 */
			tlb_invalidate(asid, va);
			/* 重新设置权限位 */
			*pte = page2pa(pp) | perm | PTE_C_CACHEABLE | PTE_V;
			return 0;
		}
	}

	/* Step 2: Flush TLB with 'tlb_invalidate'. */
	/* Exercise 2.7: Your code here. (1/3) */
	/* 同样刷新TLB缓存信息 */
	tlb_invalidate(asid, va);

	/* Step 3: Re-get or create the page table entry. */
	/* If failed to create, return the error. */
	/* Exercise 2.7: Your code here. (2/3) */
	/* 再次获取页表项 */
	/* 这次create参数为1允许创建新的页表 */
	int return_code = pgdir_walk(pgdir, va, 1, &pte);
	/* 失败返回错误码 */
	if (return_code == -E_NO_MEM) {
		return -E_NO_MEM;
	}

	/* Step 4: Insert the page to the page table entry with 'perm | PTE_C_CACHEABLE | PTE_V'
	 * and increase its 'pp_ref'. */
	/* Exercise 2.7: Your code here. (3/3) */
	/* 插入页面到页表项中 */
	*pte = page2pa(pp) | perm | PTE_C_CACHEABLE | PTE_V;
	/* 增加页面的引用计数 */
	++(pp->pp_ref);

	return 0;
}

/* Lab 2 Key Code "page_lookup" */
/** Overview:
 * Look up the Page that virtual address `va` map to.
 * Post-Condition:
 * Return a pointer to corresponding Page, 
 * and store it's page table entry to *ppte.
 * If `va` doesn't mapped to any Page, 
 * return NULL.
*/
/**
 * 寻找虚拟地址va对应的页面并将ppte指向的空间设置为二级页表项地址
 * @param pgdir 页目录基地址
 * @param va 虚拟地址
 * @param ppte 二级页表项地址存放位置
*/
struct Page *page_lookup(Pde *pgdir, u_long va, Pte **ppte) {
	struct Page *pp;
	Pte *pte;

	/* Step 1: Get the page table entry. */
	/* 获取虚拟地址va对应的页表项 */
	/* 参数create为0不允许创建新的页表 */
	pgdir_walk(pgdir, va, 0, &pte);

	/* Hint: Check if the page table entry doesn't exist or is not valid. */
	/* 检查页表项是否存在或者无效 */
	if (pte == NULL || (*pte & PTE_V) == 0) {
		return NULL;
	}

	/* Step 2: Get the corresponding Page struct. */
	/* Hint: Use function `pa2page`, defined in include/pmap.h . */
	/* 获取页表项对应的页面结构体 */
	pp = pa2page(*pte);
	/* 如果ppte不为空说明调用方需要这个数据则写入 */
	if (ppte) {
		*ppte = pte;
	}

	return pp;
}
/* End of Key Code "page_lookup" */

/* Overview:
 * Decrease the 'pp_ref' value of Page 'pp'.
 * When there's no references (mapped virtual address) to this page, release it.
*/
/**
 * 减少页面的引用次数
 * 如果没有引用次数则释放该页面
*/
void page_decref(struct Page *pp) {
	assert(pp->pp_ref > 0);

	/* If 'pp_ref' reaches to 0, free this page. */
	if (--pp->pp_ref == 0) {
		page_free(pp);
	}
}

/* Lab 2 Key Code "page_remove" */
// Overview:
// Unmap the physical page at virtual address 'va'.
/* 取消虚拟地址va到物理地址的映射 */
void page_remove(Pde *pgdir, u_int asid, u_long va) {
	Pte *pte;

	/* Step 1: Get the page table entry, and check if the page table entry is valid. */
	/* 检索虚拟地址va对应的页面 */
	struct Page *pp = page_lookup(pgdir, va, &pte);
	if (pp == NULL) {
		return;
	}

	/* Step 2: Decrease reference count on 'pp'. */
	/* 减少页面引用次数 */
	page_decref(pp);

	/* Step 3: Flush TLB. */
	/* 清空页表项 */
	*pte = 0;
	/* 清空映射在TLB中的缓存 */
	tlb_invalidate(asid, va);
	return;
}
/* End of Key Code "page_remove" */

struct Page_list buddy_free_list[2];

#define BUDDYS_COUNT ((BUDDY_PAGE_END - BUDDY_PAGE_BASE) / PAGE_SIZE / 2)

struct Page* buddys[BUDDYS_COUNT];

void buddy_init() {
	LIST_INIT(&buddy_free_list[0]);
	LIST_INIT(&buddy_free_list[1]);
	for (int i = BUDDY_PAGE_BASE; i < BUDDY_PAGE_END; i += PAGE_SIZE) {
		struct Page *pp = pa2page(i);
		LIST_REMOVE(pp, pp_link);
	}
	for (int i = BUDDY_PAGE_BASE; i < BUDDY_PAGE_END; i += 2 * PAGE_SIZE) {
		struct Page *pp = pa2page(i);
		LIST_INSERT_HEAD(&buddy_free_list[1], pp, pp_link);
	}
}

int buddy_alloc(u_int size, struct Page **new) {
	/* Your Code Here (1/2) */
	if (size <= PAGE_SIZE) {
		if (LIST_EMPTY(&buddy_free_list[0])) {
			if (LIST_EMPTY(&buddy_free_list[1])) {
				return -E_NO_MEM;
			}
			struct Page *buddy0 = LIST_FIRST(&buddy_free_list[1]);
			LIST_REMOVE(buddy0, pp_link);
			struct Page *buddy1 = buddy0 + 1;
			int index = (page2pa(buddy0) - BUDDY_PAGE_BASE) / PAGE_SIZE / 2;
			buddys[index] = buddy1;
			*new = buddy0;
			LIST_INSERT_HEAD(&buddy_free_list[0], buddy1, pp_link);
			return 1;
		} else {
			struct Page *pp = LIST_FIRST(&buddy_free_list[0]);
			LIST_REMOVE(pp, pp_link);
			int index = (page2pa(pp) - BUDDY_PAGE_BASE) / PAGE_SIZE / 2;
			buddys[index] = NULL;
			*new = pp;
			return 1;
		}
	} else {
		if (LIST_EMPTY(&buddy_free_list[1])) {
			return -E_NO_MEM;
		}
		struct Page *pp = LIST_FIRST(&buddy_free_list[1]);
		LIST_REMOVE(pp, pp_link);
		*new = pp;
		return 2;
	}	
}

void buddy_free(struct Page *pp, int npp) {
	/* Your Code Here (2/2) */
	if (npp == 1) {
		int double_index = (page2pa(pp) - BUDDY_PAGE_BASE) / PAGE_SIZE;
		int index = double_index / 2;
		struct Page *other = buddys[index];
		if (other != NULL) {
			LIST_REMOVE(other, pp_link);
			buddys[index] = NULL;
			if ((double_index & 1) == 0) {
				LIST_INSERT_HEAD(&buddy_free_list[1], pp, pp_link);
			} else {
				LIST_INSERT_HEAD(&buddy_free_list[1], other, pp_link);
			}
		} else {
			LIST_INSERT_HEAD(&buddy_free_list[0], pp, pp_link);
			buddys[index] = pp;
		}
	} else {
		LIST_INSERT_HEAD(&buddy_free_list[1], pp, pp_link);
	}
}

void physical_memory_manage_check(void) {
	struct Page *pp, *pp0, *pp1, *pp2;
	struct Page_list fl;
	int *temp;

	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	assert(page_alloc(&pp0) == 0);
	assert(page_alloc(&pp1) == 0);
	assert(page_alloc(&pp2) == 0);

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);

	// temporarily steal the rest of the free pages
	fl = page_free_list;
	// now this page_free list must be empty!!!!
	LIST_INIT(&page_free_list);
	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	temp = (int *)page2kva(pp0);
	// write 1000 to pp0
	*temp = 1000;
	// free pp0
	page_free(pp0);
	printk("The number in address temp is %d\n", *temp);

	// alloc again
	assert(page_alloc(&pp0) == 0);
	assert(pp0);

	// pp0 should not change
	assert(temp == (int *)page2kva(pp0));
	// pp0 should be zero
	assert(*temp == 0);

	page_free_list = fl;
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);
	struct Page_list test_free;
	struct Page *test_pages;
	test_pages = (struct Page *)alloc(10 * sizeof(struct Page), PAGE_SIZE, 1);
	LIST_INIT(&test_free);
	// LIST_FIRST(&test_free) = &test_pages[0];
	int i, j = 0;
	struct Page *p, *q;
	for (i = 9; i >= 0; i--) {
		test_pages[i].pp_ref = i;
		// test_pages[i].pp_link=NULL;
		// printk("0x%x  0x%x\n",&test_pages[i], test_pages[i].pp_link.le_next);
		LIST_INSERT_HEAD(&test_free, &test_pages[i], pp_link);
		// printk("0x%x  0x%x\n",&test_pages[i], test_pages[i].pp_link.le_next);
	}
	p = LIST_FIRST(&test_free);
	int answer1[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	assert(p != NULL);
	while (p != NULL) {
		// printk("%d %d\n",p->pp_ref,answer1[j]);
		assert(p->pp_ref == answer1[j++]);
		// printk("ptr: 0x%x v: %d\n",(p->pp_link).le_next,((p->pp_link).le_next)->pp_ref);
		p = LIST_NEXT(p, pp_link);
	}
	// insert_after test
	int answer2[] = {0, 1, 2, 3, 4, 20, 5, 6, 7, 8, 9};
	q = (struct Page *)alloc(sizeof(struct Page), PAGE_SIZE, 1);
	q->pp_ref = 20;

	// printk("---%d\n",test_pages[4].pp_ref);
	LIST_INSERT_AFTER(&test_pages[4], q, pp_link);
	// printk("---%d\n",LIST_NEXT(&test_pages[4],pp_link)->pp_ref);
	p = LIST_FIRST(&test_free);
	j = 0;
	// printk("into test\n");
	while (p != NULL) {
		//      printk("%d %d\n",p->pp_ref,answer2[j]);
		assert(p->pp_ref == answer2[j++]);
		p = LIST_NEXT(p, pp_link);
	}

	printk("physical_memory_manage_check() succeeded\n");
}

void page_check(void) {
	struct Page *pp, *pp0, *pp1, *pp2;
	struct Page_list fl;

	// should be able to allocate a page for directory
	assert(page_alloc(&pp) == 0);
	Pde *boot_pgdir = (Pde *)page2kva(pp);

	// should be able to allocate three pages
	pp0 = pp1 = pp2 = 0;
	assert(page_alloc(&pp0) == 0);
	assert(page_alloc(&pp1) == 0);
	assert(page_alloc(&pp2) == 0);

	assert(pp0);
	assert(pp1 && pp1 != pp0);
	assert(pp2 && pp2 != pp1 && pp2 != pp0);

	// temporarily steal the rest of the free pages
	fl = page_free_list;
	// now this page_free list must be empty!!!!
	LIST_INIT(&page_free_list);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// there is no free memory, so we can't allocate a page table
	assert(page_insert(boot_pgdir, 0, pp1, 0x0, 0) < 0);

	// free pp0 and try again: pp0 should be used for page table
	page_free(pp0);
	assert(page_insert(boot_pgdir, 0, pp1, 0x0, 0) == 0);
	assert(PTE_FLAGS(boot_pgdir[0]) == (PTE_C_CACHEABLE | PTE_V));
	assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));
	assert(PTE_FLAGS(*(Pte *)page2kva(pp0)) == (PTE_C_CACHEABLE | PTE_V));

	printk("va2pa(boot_pgdir, 0x0) is %x\n", va2pa(boot_pgdir, 0x0));
	printk("page2pa(pp1) is %x\n", page2pa(pp1));
	//  printk("pp1->pp_ref is %d\n",pp1->pp_ref);
	assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
	assert(pp1->pp_ref == 1);

	// should be able to map pp2 at PAGE_SIZE because pp0 is already allocated for page table
	assert(page_insert(boot_pgdir, 0, pp2, PAGE_SIZE, 0) == 0);
	assert(va2pa(boot_pgdir, PAGE_SIZE) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	printk("start page_insert\n");
	// should be able to map pp2 at PAGE_SIZE because it's already there
	assert(page_insert(boot_pgdir, 0, pp2, PAGE_SIZE, 0) == 0);
	assert(va2pa(boot_pgdir, PAGE_SIZE) == page2pa(pp2));
	assert(pp2->pp_ref == 1);

	// pp2 should NOT be on the free list
	// could happen in ref counts are handled sloppily in page_insert
	assert(page_alloc(&pp) == -E_NO_MEM);

	// should not be able to map at PDMAP because need free page for page table
	assert(page_insert(boot_pgdir, 0, pp0, PDMAP, 0) < 0);

	// insert pp1 at PAGE_SIZE (replacing pp2)
	assert(page_insert(boot_pgdir, 0, pp1, PAGE_SIZE, 0) == 0);

	// should have pp1 at both 0 and PAGE_SIZE, pp2 nowhere, ...
	assert(va2pa(boot_pgdir, 0x0) == page2pa(pp1));
	assert(va2pa(boot_pgdir, PAGE_SIZE) == page2pa(pp1));
	// ... and ref counts should reflect this
	assert(pp1->pp_ref == 2);
	printk("pp2->pp_ref %d\n", pp2->pp_ref);
	assert(pp2->pp_ref == 0);
	printk("end page_insert\n");

	// pp2 should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp2);

	// unmapping pp1 at 0 should keep pp1 at PAGE_SIZE
	page_remove(boot_pgdir, 0, 0x0);
	assert(va2pa(boot_pgdir, 0x0) == ~0);
	assert(va2pa(boot_pgdir, PAGE_SIZE) == page2pa(pp1));
	assert(pp1->pp_ref == 1);
	assert(pp2->pp_ref == 0);

	// unmapping pp1 at PAGE_SIZE should free it
	page_remove(boot_pgdir, 0, PAGE_SIZE);
	assert(va2pa(boot_pgdir, 0x0) == ~0);
	assert(va2pa(boot_pgdir, PAGE_SIZE) == ~0);
	assert(pp1->pp_ref == 0);
	assert(pp2->pp_ref == 0);

	// so it should be returned by page_alloc
	assert(page_alloc(&pp) == 0 && pp == pp1);

	// should be no free memory
	assert(page_alloc(&pp) == -E_NO_MEM);

	// forcibly take pp0 back
	assert(PTE_ADDR(boot_pgdir[0]) == page2pa(pp0));
	boot_pgdir[0] = 0;
	assert(pp0->pp_ref == 1);
	pp0->pp_ref = 0;

	// give free list back
	page_free_list = fl;

	// free the pages we took
	page_free(pp0);
	page_free(pp1);
	page_free(pp2);
	page_free(pa2page(PADDR(boot_pgdir)));

	printk("page_check() succeeded!\n");
}
