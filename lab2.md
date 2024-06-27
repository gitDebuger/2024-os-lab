# Lab2实验报告

## 1. 思考题

### 1.1 Thinking 2.1

**在编写的C程序中，指针变量中存储的地址被视为虚拟地址还是物理地址？**

**MIPS汇编程序中 `lw` 和 `sw` 指令使用的地址被视为虚拟地址还是物理地址？** 

虚拟地址还是物理地址与所使用的编程语言没有关系，不管是C语言这种高级语言还是汇编语言，其中的地址是虚拟地址还是物理地址只取决于程序的运行环境。

对于运行在具有虚拟内存管理机制的操作系统上的程序，其中的地址是虚拟地址；对于运行在stm32这种单片机上的嵌入式程序而言，由于硬件本身的内存空间很小，也没有大量的程序需要运行，往往只有一个程序运行在单片机上，这时使用虚拟内存机制没有任何意义，此时的地址就是物理地址。

所以，程序中的地址究竟是虚拟地址还是物理地址，需要结合实际情况进行分析，与使用的编程语言无关。

### 1.2 Thinking 2.2

* **从可重用性的角度，阐述用宏来实现链表的好处。**
* **查看实验环境中的 `/usr/include/sys/queue.h` ，了解其中单向链表与循环链表的实现，比较它们与本实验中使用的双向链表，分析三者在插入与删除操作上的性能差异。**

由于C语言不具有泛型机制，所以为了在其中达到泛型的效果，必须通过宏。从而在这里使用宏实现链表的优点，就变成了使用泛型类和泛型函数实现链表的优点。所以使用宏实现链表的优点如下：

1. 泛型性：宏可以根据不同的数据类型生成相应的代码，使得链表可以存储任意类型的数据，实现了泛型能力。
2. 灵活性：通过宏定义链表操作函数，可以在不同的程序中轻松地使用相同的链表实现，而无需为每种数据类型编写不同的链表代码。
3. 简化代码：使用宏可以减少重复的代码量，提高代码的可读性和可维护性。相比于手动编写针对不同数据类型的链表实现，使用宏可以减少冗余代码，简化链表操作函数的编写。
4. 易于维护：通过宏定义的链表操作函数，可以集中管理链表的逻辑，当需要修改链表操作时，只需修改宏定义即可，而不必修改多处代码。
5. 提高效率：使用宏定义链表操作函数可以将一些常用的操作优化为内联代码，提高程序的执行效率。

相对于单向链表和循环链表，双向链表在插入与删除操作上具有以下优势：

1. 在保证 $O(1)$​ 时间复杂度的前提下，双向链表中可以在任意位置进行前插和后插操作，即在某个节点之前或之后插入新节点，而不仅仅局限于在链表头部或尾部插入节点。这使得双向链表更加灵活，可以更方便地实现特定的插入需求。
2. 对于已知节点的情况下，双向链表的删除操作效率更高。因为双向链表中的节点包含指向前一个节点和后一个节点的指针，所以在删除某个节点时，可以直接修改其前一个节点和后一个节点的指针，而不需要像单向链表那样需要遍历找到前一个节点。这使得双向链表的删除操作更加高效。

### 1.3 Thinking 2.3

**请阅读 `include/queue.h` 以及 `include/pmap.h` , 将 `Page_list` 的结构梳理清楚，选择正确的展开结构。**

首先，有下面的三个定义：

```c
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
```

可以得出 `Page` 结构体的展开如下：

```c
struct Page {
	struct {
		struct Page *le_next;
		struct Page **le_prev;
	} pp_link;
	u_short pp_ref;
};
```

进而再结合下面的宏定义和宏的调用：

```c
#define LIST_HEAD(name, type) \
	struct name { \
		struct type *lh_first; \
	}
LIST_HEAD(Page_list, Page);
```

可以得出 `Page_list` 结构体的定义：

```c
struct Page_list {
	struct Page *lh_first;
};
```

从而 `Page_list` 结构体的展开如下：

```c
struct Page_list {
	struct {
		struct {
			struct Page *le_next;
			struct Page **le_prev;
		} pplink;
		u_short pp_ref;
	} *lh_first;
};
```

所以，应该选择题目中的 C 选项。

### 1.4 Thinking 2.4

* **请阅读上面有关TLB的描述，从虚拟内存和多进程操作系统的实现角度，阐述ASID的必要性。**
* **请阅读MIPS 4Kc文档《MIPS32® 4K™ Processor Core Family Software User’s Manual》的Section 3.3.1与Section 3.4，结合ASID段的位数，说明4Kc中可容纳不同的地址空间的最大数量。**

ASID是一个用于标识进程地址空间的唯一标识符。在多进程操作系统中，每个进程都有自己的地址空间，包括代码、数据和堆栈等。

ASID在TLB中的必要性主要体现在以下几个方面：

1. **隔离进程地址空间：**在多进程环境中，不同的进程具有不同的地址空间，因此需要确保TLB中缓存的虚拟地址到物理地址的映射关系是针对特定进程的。通过为每个进程分配独立的ASID，可以将TLB的缓存划分为多个部分，确保不同进程之间的地址映射不会互相干扰，从而实现进程间的地址空间隔离。
2. **提高TLB的效率：**如果TLB中不使用ASID来区分不同的地址空间，那么在进程切换时，必须将TLB中的所有条目都清空，以防止新进程使用之前进程的地址映射。这样做会降低TLB的命中率，影响系统性能。而使用ASID可以使TLB中的条目与进程关联，进程切换时只需要刷新与当前进程相关的TLB条目，可以减少TLB刷新的开销，提高TLB的命中率和整体系统性能。
3. **简化地址转换逻辑：**在硬件实现中，TLB的地址转换逻辑需要根据当前虚拟地址和ASID来查找对应的物理地址。有了ASID，可以简化TLB的查找逻辑，只需在TLB查找时同时匹配虚拟地址和ASID即可，而不必考虑多个进程之间的地址映射关系。

在MIPS 4Kc中，ASID字段的位数通常是 $8$ 位。这意味着可以有 $2^8=256$ 个不同的ASID。但是由于TLB的大小是固定的，因此可以容纳的不同地址空间的最大数量受到TLB的大小限制。根据MIPS 4Kc文档中Section 3.3.1和Section 3.4的描述，MIPS 4Kc中的TLB共有 $64$ 个条目。每个TLB条目可以映射一个虚拟地址到一个物理地址。

综上所述，MIPS 4Kc中可以容纳的不同地址空间的最大数量受到TLB条目的数量限制，即 $64$ 个。因此，即使ASID有 $256$ 个不同的取值，但实际上，MIPS 4Kc可以同时支持的不同地址空间的最大数量仍然是 $64$ 个，这是由TLB的大小所决定的。

### 1.5 Thinking 2.5

**请回答下述三个问题：**

* **`tlb_invalidate` 和 `tlb_out` 的调用关系？**
* **请用一句话概括 `tlb_invalidate` 的作用。**
* **逐行解释 `tlb_out` 中的汇编代码。**

C语言源代码文件 `tlbex.c` 中的 `tlb_invalidate` 函数会调用MIPS汇编文件 `tlb_asm.S` 中的叶函数 `tlb_out` 。

函数 `tlb_invalidate` 的作用是：在更新页表项后被调用，以将给定虚拟地址 `va` 和标识符 `asid` 对应的TLB表项无效化，从而使得在下一次访问该虚拟地址时会触发TLB重填异常，使得操作系统对TLB进行重填。

逐行解释 `tlb_out` 中的汇编代码：

```assembly
/* 定义函数入口点即函数的标签 */
/* 这里定义了一个名为 tlb_out 的函数 */
LEAF(tlb_out)
/* 告诉汇编器禁止对指令重新排序 */
/* 确保在后续的代码中指令的顺序不会被改变。 */
.set noreorder
	/* 将协处理器 0 中寄存器 CP0_ENTRYHI 中的数据写入到通用寄存器 t0 中  */
	mfc0    t0, CP0_ENTRYHI
	/* 将通用寄存器 a0 中的数据写入到协处理器 0 中寄存器 CP0_ENTRYHI 中 */
	mtc0    a0, CP0_ENTRYHI
	/* 添加空指令避免数据冲突 */
	nop
	/* Step 1: Use 'tlbp' to probe TLB entry */
	/* Exercise 2.8: Your code here. (1/2) */
	/* 根据 EntryHi 寄存器中的 Key 值查找 TLB 中对应表项 */
	/* 并将表项的索引存入 Index 寄存器 */
	tlbp
	/* 添加空指令避免数据冲突 */
	nop
	/* Step 2: Fetch the probe result from CP0.Index */
	/* 将协处理器 0 中寄存器 CP0_INDEX 中的数据写入到通用寄存器 t1 中 */
	/* 也就是将表项索引从寄存器 CP0_INDEX 存入寄存器 t1 中 */
	mfc0    t1, CP0_INDEX
/* 告诉汇编器恢复对指令的重新排序 */
.set reorder
	/* 如果 t1 的值小于零则跳转到标签 NO_SUCH_ENTRY 处执行 */
	/* 这里用于检查 TLB 查询结果是否为无效 */
	/* 即没有匹配的 TLB 表项 */
	bltz    t1, NO_SUCH_ENTRY
/* 告诉汇编器禁止对指令重新排序 */
.set noreorder
	/* 如果存在对应的表项执行无效化操作 */
	/* 清空 CP0 中的 ENTRYHI ENTRYLO0 和 ENTRYLO1 寄存器 */
	/* 相当于将这些寄存器中的内容置零 */
	mtc0    zero, CP0_ENTRYHI
	mtc0    zero, CP0_ENTRYLO0
	mtc0    zero, CP0_ENTRYLO1
	/* 插入空指令避免数据冲突 */
	nop
	/* Step 3: Use 'tlbwi' to write CP0.EntryHi/Lo into TLB at CP0.Index  */
	/* Exercise 2.8: Your code here. (2/2) */
	/* 以 Index 寄存器中的值为索引将 EntryHi 与 EntryLo 中的数据写入到 TLB 中对应表项 */
	tlbwi
/* 告诉汇编器恢复对指令的重新排序 */
.set reorder
/* 定义了一个标签 */
/* 用于表示如果不存在匹配的 TLB 表项时跳转到的位置 */
NO_SUCH_ENTRY:
	/* 将通用寄存器 t0 的内容写入到协处理器 0 中的寄存器 CP0_ENTRYHI 中 */
	/* 相当于恢复寄存器 CP0_ENTRYHI 的值 */
	mtc0    t0, CP0_ENTRYHI
	/* 无条件跳转指令 */
	/* 用于跳转到寄存器 ra 中保存的返回地址 */
	/* 用于函数返回 */
	j       ra
/* 定义函数的结束点 */
END(tlb_out)
```

### 1.6 Thinking 2.6

**从下述三个问题中任选其一回答：**

1. **简单了解并叙述x86体系结构中的内存管理机制，比较x86和MIPS在内存管理上的区别。**
2. **简单了解并叙述RISC-V中的内存管理机制，比较RISC-V与MIPS在内存管理上的区别。**
3. **简单了解并叙述LoongArch中的内存管理机制，比较LoongArch与MIPS在内存管理上的区别。**

在x86体系结构中，内存管理主要通过分段和分页来实现。分段将物理内存划分为不同的段，每个段有自己的基址和界限，用于实现逻辑地址到线性地址的转换。分页通过页表将线性地址映射到物理地址，实现了虚拟地址到物理地址的转换。x86体系结构还支持虚拟内存和页面置换机制，以及特权级别的保护机制。

与MIPS相比，x86体系结构采用了较为复杂的分段机制，其中包括全局描述符表、局部描述符表等。而MIPS主要采用分页机制，对内存管理相对简单。此外，x86体系结构在内存管理方面提供了更多的特性和功能，如分页级别的保护、支持大页、内存类型等，使其在内存管理方面更加灵活和强大。

## 2. 难点分析

### 2.1 内存分配与管理

内存分配与管理的相关函数主要位于 `kern/pmap.c` 中，其中包括可用物理空间的探测、建立页式管理机制之前的内存分配、页式管理机制初始化、页面申请、页面回收、页面寻找等。这些函数是整个页面管理机制的核心，需要读懂每个函数的功能才能理解整个内存分配与管理机制。

```c
void mips_detect_memory(u_int _memsize);
void *alloc(u_int n, u_int align, int clear);
void mips_vm_init();
void page_init(void);
int page_alloc(struct Page **new);
void page_free(struct Page *pp);
static int pgdir_walk(Pde *pgdir, u_long va, int create, Pte **ppte);
int page_insert(Pde *pgdir, u_int asid, struct Page *pp, u_long va, u_int perm);
struct Page *page_lookup(Pde *pgdir, u_long va, Pte **ppte);
void page_decref(struct Page *pp);
void page_remove(Pde *pgdir, u_int asid, u_long va);
```

以上就是内存管理机制的核心函数。

### 2.2 链表宏

在 `include/queue.h` 中使用宏实现了泛型的双向链表。

由于C语言本身不支持泛型，需要通过宏模拟，会使得代码略显晦涩。另外，这里的双向链表不同于常规的双向链表，它的前向指针实际上是指向了前一个链表节点的指针域，也就是**指向了"指向自己的指针"的指针**。

理解了这一点后，就能理解这个文件中对于双向链表的实现原理了。

### 2.3 TLB

TLB结构和操作流程比较复杂，同时MIPS的TLB使用了奇偶页的设计，使得其更加难以理解。

主要操作函数位于 `kern/tlbex.c` 和 `kern/tlb_asm.S` 中，其中C语言代码包括的功能有TLB无效化、被动页面分配和TLB重填等功能。

汇编语言中，主要涉及对三个寄存器的操作以及几个特殊的TLB操作指令。

## 3. 实验体会

操作系统实验涉及的概念和程序代码较多，一定要读懂理解整个工程项目的代码，才能完成好每一次实验。

另外，限时测试中需要编写的代码和已有的代码结构很相似，了解原有代码的实现方式有助于在限时测试中完成代码的编写。

