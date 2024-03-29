#ifndef _MMU_H_
#define _MMU_H_

/*
 * Part 1.  Page table/directory defines.
 */

#define NASID 256 /* unknown */
#define PAGE_SIZE 4096 /* 页面大小 */
#define PTMAP PAGE_SIZE /* 页面大小 */
#define PDMAP (4 * 1024 * 1024) // bytes mapped by a page directory entry 由页目录表映射的字节即页目录表的大小
#define PGSHIFT 12 /* 页大小 log_2 对数值 */
#define PDSHIFT 22 // log_2(PDMAP)
#define PDX(va) ((((u_long)(va)) >> PDSHIFT) & 0x03FF) /* 虚拟地址对应的页目录项相对于相对于页目录基地址的偏移 */
#define PTX(va) ((((u_long)(va)) >> PGSHIFT) & 0x03FF) /* 虚拟地址对应的页目录项相对于页表基地址的偏移 */
#define PTE_ADDR(pte) (((u_long)(pte)) & ~0xFFF) /* 获取页表项中的物理页帧地址 */
#define PTE_FLAGS(pte) (((u_long)(pte)) & 0xFFF) /* 获取页表项中的标志位集合 */

// Page number field of an address
#define PPN(pa) (((u_long)(pa)) >> PGSHIFT) /* 提取给定物理地址的页号 */
#define VPN(va) (((u_long)(va)) >> PGSHIFT) /* 提取给定虚拟地址的页号 */

// Page Table/Directory Entry flags

#define PTE_HARDFLAG_SHIFT 6 /* 硬件标志位相对于页表项的起始地址 */

// TLB EntryLo and Memory Page Table Entry Bit Structure Diagram.
// entrylo.value == pte.value >> 6
/*
 * +----------------------------------------------------------------+
 * |                     TLB EntryLo Structure                      |
 * +-----------+---------------------------------------+------------+
 * |3 3 2 2 2 2|2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0|0 0 0 0 0 0 |
 * |1 0 9 8 7 6|5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6|5 4 3 2 1 0 |
 * +-----------+---------------------------------------+------------+------------+
 * | Reserved  |         Physical Frame Number         |  Hardware  |  Software  |
 * |Zero Filled|                20 bits                |    Flag    |    Flag    |
 * +-----------+---------------------------------------+------------+------------+
 *             |3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1|1 1 0 0 0 0 |0 0 0 0 0 0 |
 *             |1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2|1 0 9 8 7 6 |5 4 3 2 1 0 |
 *             |                                       |    C D V G |            |
 *             +---------------------------------------+------------+------------+
 *             |                Memory Page Table Entry Structure                |
 *             +-----------------------------------------------------------------+
 */

// Global bit. When the G bit in a TLB entry is set, that TLB entry will match solely on the VPN
// field, regardless of whether the TLB entry’s ASID field matches the value in EntryHi.
#define PTE_G (0x0001 << PTE_HARDFLAG_SHIFT)

// Valid bit. If 0 any address matching this entry will cause a tlb miss exception (TLBL/TLBS).
#define PTE_V (0x0002 << PTE_HARDFLAG_SHIFT)

// Dirty bit, but really a write-enable bit. 1 to allow writes, 0 and any store using this
// translation will cause a tlb mod exception (TLB Mod).
#define PTE_D (0x0004 << PTE_HARDFLAG_SHIFT)

// Cache Coherency Attributes bit.
#define PTE_C_CACHEABLE (0x0018 << PTE_HARDFLAG_SHIFT)
#define PTE_C_UNCACHEABLE (0x0010 << PTE_HARDFLAG_SHIFT)

// Copy On Write. Reserved for software, used by fork.
#define PTE_COW 0x0001

// Shared memmory. Reserved for software, used by fork.
#define PTE_LIBRARY 0x0002

// Memory segments (32-bit kernel mode addresses)
#define KUSEG 0x00000000U
#define KSEG0 0x80000000U
#define KSEG1 0xA0000000U
#define KSEG2 0xC0000000U

/*
 * Part 2.  Our conventions.
 */

/*
 o     4G ----------->  +----------------------------+------------0x100000000
 o                      |       ...                  |  kseg2
 o      KSEG2    -----> +----------------------------+------------0xc000 0000
 o                      |          Devices           |  kseg1
 o      KSEG1    -----> +----------------------------+------------0xa000 0000
 o                      |      Invalid Memory        |   /|\
 o                      +----------------------------+----|-------Physical Memory Max
 o                      |       ...                  |  kseg0
 o      KSTACKTOP-----> +----------------------------+----|-------0x8040 0000-------end
 o                      |       Kernel Stack         |    | KSTKSIZE            /|\
 o                      +----------------------------+----|------                |
 o                      |       Kernel Text          |    |                    PDMAP
 o      KERNBASE -----> +----------------------------+----|-------0x8002 0000    |
 o                      |      Exception Entry       |   \|/                    \|/
 o      ULIM     -----> +----------------------------+------------0x8000 0000-------
 o                      |         User VPT           |     PDMAP                /|\
 o      UVPT     -----> +----------------------------+------------0x7fc0 0000    |
 o                      |           pages            |     PDMAP                 |
 o      UPAGES   -----> +----------------------------+------------0x7f80 0000    |
 o                      |           envs             |     PDMAP                 |
 o  UTOP,UENVS   -----> +----------------------------+------------0x7f40 0000    |
 o  UXSTACKTOP -/       |     user exception stack   |     PTMAP                 |
 o                      +----------------------------+------------0x7f3f f000    |
 o                      |                            |     PTMAP                 |
 o      USTACKTOP ----> +----------------------------+------------0x7f3f e000    |
 o                      |     normal user stack      |     PTMAP                 |
 o                      +----------------------------+------------0x7f3f d000    |
 a                      |                            |                           |
 a                      ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~                           |
 a                      .                            .                           |
 a                      .                            .                         kuseg
 a                      .                            .                           |
 a                      |~~~~~~~~~~~~~~~~~~~~~~~~~~~~|                           |
 a                      |                            |                           |
 o       UTEXT   -----> +----------------------------+------------0x0040 0000    |
 o                      |      reserved for COW      |     PTMAP                 |
 o       UCOW    -----> +----------------------------+------------0x003f f000    |
 o                      |   reversed for temporary   |     PTMAP                 |
 o       UTEMP   -----> +----------------------------+------------0x003f e000    |
 o                      |       invalid memory       |                          \|/
 a     0 ------------>  +----------------------------+ ----------------------------
 o
*/

#define KERNBASE 0x80020000 /* 内核基地址 */

#define KSTACKTOP (ULIM + PDMAP) /* 内核堆栈的顶部地址 */
#define ULIM 0x80000000

#define UVPT (ULIM - PDMAP)
#define UPAGES (UVPT - PDMAP)
#define UENVS (UPAGES - PDMAP)

#define UTOP UENVS
#define UXSTACKTOP UTOP

#define USTACKTOP (UTOP - 2 * PTMAP)
#define UTEXT PDMAP
#define UCOW (UTEXT - PTMAP)
#define UTEMP (UCOW - PTMAP)

#ifndef __ASSEMBLER__

/*
 * Part 3.  Our helper functions.
 */
#include <error.h>
#include <string.h>
#include <types.h>

extern u_long npage; /* 内核总页面数 */

typedef u_long Pde; /* 页目录项 page directory entry */
typedef u_long Pte; /* 页表项 page table entry */

/* 从内核虚拟地址转化为物理地址 */
#define PADDR(kva)                                                                                 \
	({                                                                                         \
		u_long _a = (u_long)(kva);                                                         \
		if (_a < ULIM)                                                                     \
			panic("PADDR called with invalid kva %08lx", _a);                          \
		_a - ULIM;                                                                         \
	})

// translates from physical address to kernel virtual address
/* 从物理地址转化为内核虚拟地址 */
#define KADDR(pa)                                                                                  \
	({                                                                                         \
		u_long _ppn = PPN(pa);                                                             \
		if (_ppn >= npage) {                                                               \
			panic("KADDR called with invalid pa %08lx", (u_long)pa);                   \
		}                                                                                  \
		(pa) + ULIM;                                                                       \
	})

/* 断言 */
#define assert(x)                                                                                  \
	do {                                                                                       \
		if (!(x)) {                                                                        \
			panic("assertion failed: %s", #x);                                         \
		}                                                                                  \
	} while (0)

#define TRUP(_p)                                                                                   \
	({                                                                                         \
		typeof((_p)) __m_p = (_p);                                                         \
		(u_int) __m_p > ULIM ? (typeof(_p))ULIM : __m_p;                                   \
	})

extern void tlb_out(u_int entryhi);
void tlb_invalidate(u_int asid, u_long va);
#endif //!__ASSEMBLER__
#endif // !_MMU_H_
