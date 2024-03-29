/*
 * .lds 文件通常是链接器脚本文件，
 * 用来指导链接器如何将各种目标文件组合成最终的可执行文件或库文件的，
 * 链接器脚本定义了程序的内存布局，
 * 包括代码、数据和符号的放置方式。
 */

/*
 * Set the architecture to mips.
 * 输出文件的体系结构为 MIPS
 */
OUTPUT_ARCH(mips)

/*
 * Set the ENTRY point of the program to _start.
 * 设定程序入口点为 _start
 */
ENTRY(_start)

/*
 * 定义各个段的布局和内容
 */
SECTIONS {
	/* Exercise 3.10: Your code here. */

	/* fill in the correct address of the key sections: text, data, bss. */
	/* Hint: The loading address can be found in the memory layout. And the data section
	 *       and bss section are right after the text section, so you can just define
	 *       them after the text section.
	 */
	/* Step 1: Set the loading address of the text section to the location counter ".". */
	/* Exercise 1.2: Your code here. (1/4) */
	/* 将当前地址设置为 0x80020000 */
	/* 这是文本段的加载地址 */
        . = 0x80020000;
	/* Step 2: Define the text section. */
	/* Exercise 1.2: Your code here. (2/4) */
	/* 定义文本段 */
	/* 包含程序的可执行代码 */
	/* 使用通配符匹配所有的 .text 节内容 */
        .text : { *(.text) }
	/* Step 3: Define the data section. */
	/* Exercise 1.2: Your code here. (3/4) */
	/* 定义数据段 */
	/* 包含程序的已初始化数据 */
        .data : { *(.data) }
	/* 定义未初始化数据段 .bss 的起始地址为当前地址 */
	bss_start = .;
	/* Step 4: Define the bss section. */
	/* Exercise 1.2: Your code here. (4/4) */
	/* 定义 .bss 段 */
	/* 包含未初始化的全局变量 */
        .bss : { *(.bss) }
	/* 定义 .bss 段的结束地址为当前地址 */
	bss_end = .;
	/* 定义当前地址为 0x80400000 */
	. = 0x80400000;
	/* 定义符号 end 表示程序的结束地址 */
	end = . ;
}
