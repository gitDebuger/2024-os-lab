#include <elf.h>
#include <pmap.h>

/* 获取 ELF 头表指针 */
/* 如果传入的指针指向的不是 ELF 文件则返回空指针 */
/* 和 ../tools/readelf/readelf.c 中的 is_elf_format() 函数很相似 */
const Elf32_Ehdr *elf_from(const void *binary, size_t size) {
	const Elf32_Ehdr *ehdr = (const Elf32_Ehdr *)binary;
	if (size >= sizeof(Elf32_Ehdr) && ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
	    ehdr->e_ident[EI_MAG1] == ELFMAG1 && ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
	    ehdr->e_ident[EI_MAG3] == ELFMAG3 && ehdr->e_type == 2) {
		return ehdr;
	}
	return NULL;
}

/* Overview:
 *   load an elf format binary file. Map all section
 * at correct virtual address.
 *
 * Pre-Condition:
 *   `binary` can't be NULL and `size` is the size of binary.
 *
 * Post-Condition:
 *   Return 0 if success. Otherwise return < 0.
 *   If success, the entry point of `binary` will be stored in `start`
 */
/* 加载程序头表指向的程序 */
int elf_load_seg(Elf32_Phdr *ph, const void *bin, elf_mapper_t map_page, void *data) {
	/* virtual address */
	u_long va = ph->p_vaddr;
	/* file binary size */
	size_t bin_size = ph->p_filesz;
	/* memory size */
	size_t sgsize = ph->p_memsz;
	/* permutation */
	/* 有效位标记 */
	u_int perm = PTE_V;
	/* 如果程序头表的 flags 变量的 PF_W 标记位表示可写入 */
	/* 则在 permutation 权限中加入 PTE_D 可写入 */
	if (ph->p_flags & PF_W) {
		perm |= PTE_D;
	}

	int r;
	size_t i;
	/* 偏移量 offset 等于 va 减去 va 按页大小向下取整 */
	u_long offset = va - ROUNDDOWN(va, PAGE_SIZE);
	/* WHAT is map_page ? */
	/* 先将未填充满的页面填充满 */
	if (offset != 0) {
		if ((r = map_page(data, va, offset, perm, bin,
				  MIN(bin_size, PAGE_SIZE - offset))) != 0) {
			return r;
		}
	}

	/* 然后从下一个新页继续填充直到所有内容填充到内存中 */
	/* Step 1: load all content of bin into memory. */
	for (i = offset ? MIN(bin_size, PAGE_SIZE - offset) : 0; i < bin_size; i += PAGE_SIZE) {
		if ((r = map_page(data, va + i, 0, perm, bin + i, MIN(bin_size - i, PAGE_SIZE))) !=
		    0) {
			return r;
		}
	}

	/* Step 2: alloc pages to reach `sgsize` when `bin_size` < `sgsize`. */
	/* 当 bin_size 小于 sgsize 时继续填充内存空间 */
	/* 直到二者相等 */
	while (i < sgsize) {
		if ((r = map_page(data, va + i, 0, perm, NULL, MIN(bin_size - i, PAGE_SIZE))) !=
		    0) {
			return r;
		}
		i += PAGE_SIZE;
	}
	/* 看起来 map_page 是一个向内存填充程序段的函数 */
	/* @param: data 目前还不知道是用来做什么的 */
	/* @param: virtual_address 填充到内存的虚拟地址 */
	/* @param: offset 相对于页面起始地址的偏移量 */
	/* @param: permutation 程序段的权限信息 */
	/* @param: bin 需要填充的字节数据  如果传入 NULL 则填充为 0 */
	/* @param: count 填充到内存的字节数 */
	return 0;
}
