#include "elf.h"
#include <stdio.h>

/* Overview:
 *   Check whether specified buffer is valid ELF data.
 *
 * Pre-Condition:
 *   The memory within [binary, binary+size) must be valid to read.
 *
 * Post-Condition:
 *   Returns 0 if 'binary' isn't an ELF, otherwise returns 1.
 */
int is_elf_format(const void *binary, size_t size) {
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)binary;
	/* 检测要求 */
	/* 1. 文件大小大于等于 ELF 头表大小 */
	/* 2. 魔数 0 */
	/* 3. 魔数 1 */
	/* 4. 魔数 2 */
	/* 5. 魔数 3 */
	return size >= sizeof(Elf32_Ehdr) && ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
	       ehdr->e_ident[EI_MAG1] == ELFMAG1 && ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
	       ehdr->e_ident[EI_MAG3] == ELFMAG3;
}

/* Overview:
 *   Parse the sections from an ELF binary.
 *
 * Pre-Condition:
 *   The memory within [binary, binary+size) must be valid to read.
 *
 * Post-Condition:
 *   Return 0 if success. Otherwise return < 0.
 *   If success, output the address of every section in ELF.
 */

int readelf(const void *binary, size_t size) {
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)binary;

	// Check whether `binary` is a ELF file.
	/* 检测传入的字节流是否为 ELF 文件 */
	if (!is_elf_format(binary, size)) {
		fputs("not an elf file\n", stderr);
		return -1;
	}

	// Get the address of the section table, the number of section headers and the size of a
	// section header.
	const void *sh_table;
	Elf32_Half sh_entry_count;
	Elf32_Half sh_entry_size;
	/* Exercise 1.1: Your code here. (1/2) */
	/* 节头表第一项所在位置等于 ELF 头地址加上 Ehdr::e_shoff 的值 */
	/* uintptr_t 提供了特定平台能够支持的最大无符号整数类型 */
	/* 从而确保地址值不会溢出 */
	sh_table = (const void *)((uintptr_t)binary + ehdr->e_shoff);
	/* 节头表项的个数 */
	sh_entry_count = ehdr->e_shnum;
	/* 节头表项的大小 */
	sh_entry_size = ehdr->e_shentsize;

	// For each section header, output its index and the section address.
	// The index should start from 0.
	const Elf32_Shdr *section_head_table = (const Elf32_Shdr *)sh_table;
	for (int i = 0; i < sh_entry_count; i++) {
		const Elf32_Shdr *shdr;
		unsigned int addr;
		/* Exercise 1.1: Your code here. (2/2) */
		shdr = (const Elf32_Shdr *)((uintptr_t)sh_table + i * sh_entry_size);
		/* e_shentsize 字段的值通常等于 sizeof(Elf32_Shdr) 或者 sizeof(Elf64_Shdr) 的值 */
		/* 所以说其实上面的代码也可以这样写 */
		/* 节头表中就是连续排列的节头表项 */
		/* 节数据不在节头表部分 */
		/* 而是需要通过节头表中的 Elf32_Shdr::sh_addr 字段获取节本体的地址 */
		shdr = &(section_head_table[i]);
		addr = shdr->sh_addr;

		printf("%d:0x%x\n", i, addr);
	}

	return 0;
}
