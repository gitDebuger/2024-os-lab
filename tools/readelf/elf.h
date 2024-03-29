/* This file defines standard ELF types, structures, and macros.
   Copyright (C) 1995, 1996, 1997, 1998, 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ian Lance Taylor <ian@cygnus.com>.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef _ELF_H
#define _ELF_H

/* 定义标准整数类型 */
#include <stdint.h>

/* ELF defination file from GNU C Library. We simplefied this
 * file for our lab, removing definations about ELF64, structs and
 * enums which we don't care.
 */

/* 从 GNU C 标准库中获取的源代码 */
/* 移除了关于 ELF64 的定义以及该实验使用不到的结构体和枚举类型 */

/* Type for a 16-bit quantity.  */
/* 无符号16位整数 */
typedef uint16_t Elf32_Half;

/* Types for signed and unsigned 32-bit quantities.  */

/* 无符号32位整数 */
typedef uint32_t Elf32_Word;
/* 有符号32位整数 */
typedef int32_t Elf32_Sword;

/* Types for signed and unsigned 64-bit quantities.  */

/* 无符号64位整数 */
typedef uint64_t Elf32_Xword;
/* 有符号64位整数 */
typedef int64_t Elf32_Sxword;

/* Type of addresses.  */
/* 无符号32位整数作为地址类型 */
typedef uint32_t Elf32_Addr;

/* Type of file offsets.  */
/* 无符号32位整数作为文件偏移量类型 */
typedef uint32_t Elf32_Off;

/* Type for section indices, which are 16-bit quantities.  */
/* 无符号16位整数作为节标记类型 */
typedef uint16_t Elf32_Section;

/* Type of symbol indices.  */
/* 无符号32位整数作为符号标记类型 */
typedef uint32_t Elf32_Symndx;

/* Lab 1 Key Code "readelf-struct-def" */

/* The ELF file header.  This appears at the start of every ELF file.  */

#define EI_NIDENT (16)

/* Elf32_Ehdr */
typedef struct {
	/* ELF 文件标识信息 */
	unsigned char e_ident[EI_NIDENT]; /* Magic number and other info */
	/* ELF 文件类型 */
	/* 可执行文件或目标文件或共享目标文件等 */
	Elf32_Half e_type;		  /* Object file type */
	/* 目标体系结构 */
	/* 文件所针对的处理器体系结构 */
	Elf32_Half e_machine;		  /* Architecture */
	/* ELF 文件版本号 */
	/* 指示 ELF 文件的格式版本 */
	Elf32_Word e_version;		  /* Object file version */
	/* 程序入口点的虚拟地址 */
	Elf32_Addr e_entry;		  /* Entry point virtual address */
	/* 程序头表偏移量 */
	Elf32_Off e_phoff;		  /* Program header table file offset */
	/* 节头表偏移量 */
	Elf32_Off e_shoff;		  /* Section header table file offset */
	/* 处理器特定标志 */
	/* 指定一些处理器相关的特性 */
	Elf32_Word e_flags;		  /* Processor-specific flags */
	/* ELF 文件头大小 */
	Elf32_Half e_ehsize;		  /* ELF header size in bytes */
	/* 程序头表中每个条目的大小和数量 */
	Elf32_Half e_phentsize;		  /* Program header table entry size */
	Elf32_Half e_phnum;		  /* Program header table entry count */
	/* 节头表中每个条目的大小和数量 */
	Elf32_Half e_shentsize;		  /* Section header table entry size */
	Elf32_Half e_shnum;		  /* Section header table entry count */
	/* 节头表字符串表的索引 */
	Elf32_Half e_shstrndx;		  /* Section header string table index */
} Elf32_Ehdr;

/* Fields in the e_ident array.  The EI_* macros are indices into the
   array.  The macros under each EI_* macro are the values the byte
   may have.  */

/* 定义 ELF 文件头部中 e_ident 数组中各个字节的含义和对应的取值 */
/* e_ident[0] = 0x7f */
#define EI_MAG0 0    /* File identification byte 0 index */
#define ELFMAG0 0x7f /* Magic number byte 0 */

/* e_ident[1] = 'E' */
#define EI_MAG1 1   /* File identification byte 1 index */
#define ELFMAG1 'E' /* Magic number byte 1 */

/* e_ident[2] = 'L' */
#define EI_MAG2 2   /* File identification byte 2 index */
#define ELFMAG2 'L' /* Magic number byte 2 */

/* e_ident[3] = 'F' */
#define EI_MAG3 3   /* File identification byte 3 index */
#define ELFMAG3 'F' /* Magic number byte 3 */

/* Section segment header.  */
/* 节头 Section Header */
/* Elf32_Shdr */
typedef struct {
	/* 节的名称在节名称字符串表中的偏移量 */
	Elf32_Word sh_name;	 /* Section name */
	/* 节的类型指明了节的内容和语义 */
	/* SHT_NULL 未使用的节 */
	/* SHT_PROGBITS 包含程序信息的节 */
	/* SHT_SYMTAB 符号表节 */
	Elf32_Word sh_type;	 /* Section type */
	/* 节的标志指明了节的属性 */
	/* SHF_WRITE 可写 */
	/* SHF_ALLOC 可加载到内存 */
	/* SHF_EXECINSTR 可执行 */
	Elf32_Word sh_flags;	 /* Section flags */
	/* 节在内存中的虚拟地址 */
	Elf32_Addr sh_addr;	 /* Section addr */
	/* 节在文件中的偏移量 */
	Elf32_Off sh_offset;	 /* Section offset */
	/* 节的大小 */
	Elf32_Word sh_size;	 /* Section size */
	/* 节索引指向相关联的节 */
	Elf32_Word sh_link;	 /* Section link */
	/* 与节相关的其他信息 */
	Elf32_Word sh_info;	 /* Section extra info */
	/* 节在内存中的对齐要求 */
	Elf32_Word sh_addralign; /* Section alignment */
	/* 如果节包含固定大小的条目那么 sh_entsize 就是这些条目的大小 */
	/* 如果节的条目大小是可变的或者节不包含条目那么 sh_entsize 就是 0 */
	Elf32_Word sh_entsize;	 /* Section entry size */
} Elf32_Shdr;

/* Program segment header.  */
/* 程序头 Program Header */

/* Elf32_Phdr */
typedef struct {
	/* 段的类型用于指明段的用途和语义 */
	/* PT_NULL 未使用的段 */
	/* PT_LOAD 可加载段 */
	/* PT_DYNAMIC 动态链接信息段 */
	Elf32_Word p_type;   /* Segment type */
	/* 表示段在文件中的偏移量即段的数据在文件中的位置 */
	Elf32_Off p_offset;  /* Segment file offset */
	/* 段在内存中的虚拟地址 */
	Elf32_Addr p_vaddr;  /* Segment virtual address */
	/* 段在内存中的物理地址 */
	Elf32_Addr p_paddr;  /* Segment physical address */
	/* 段在文件中的大小 */
	Elf32_Word p_filesz; /* Segment size in file */
	/* 段在内存中的大小 */
	/* p_filesz 可能小于等于 p_memsz */
	/* 表示段在内存中的数据需要进行初始化或者填充 */
	Elf32_Word p_memsz;  /* Segment size in memory */
	/* 段的标志用于指定段的属性和特性 */
	/* PF_X 可执行 */
	/* PF_W 可写 */
	/* PF_R 可读 */
	Elf32_Word p_flags;  /* Segment flags */
	/* 段在内存中的对齐要求 */
	/* 通常是一个字节的对齐或者更大的对齐值 */
	Elf32_Word p_align;  /* Segment alignment */
} Elf32_Phdr;
/* End of Key Code "readelf-struct-def" */

/* Legal values for p_type (segment type).  */

/* 程序头表项未使用 */
#define PT_NULL 0	     /* Program header table entry unused */
/* 程序段可加载 */
#define PT_LOAD 1	     /* Loadable program segment */
/* 动态链接信息用于动态链接器对可执行文件进行重定位 */
#define PT_DYNAMIC 2	     /* Dynamic linking information */
/* 程序解释器指明用于解释执行该可执行文件的程序 */
#define PT_INTERP 3	     /* Program interpreter */
/* 辅助信息通常包含调试信息或其他辅助性的元数据 */
#define PT_NOTE 4	     /* Auxiliary information */
/* 保留字段暂时未使用 */
#define PT_SHLIB 5	     /* Reserved */
/* 程序头表本身的条目 */
/* 包含程序头表的描述信息 */
#define PT_PHDR 6	     /* Entry for header table itself */
/* 定义段类型的数量 */
#define PT_NUM 7	     /* Number of defined types.  */
/* 定义操作系统特定的段类型 */
#define PT_LOOS 0x60000000   /* Start of OS-specific */
#define PT_HIOS 0x6fffffff   /* End of OS-specific */
/* 定义处理器特定的段类型 */
#define PT_LOPROC 0x70000000 /* Start of processor-specific */
#define PT_HIPROC 0x7fffffff /* End of processor-specific */

/* Legal values for p_flags (segment flags).  */

#define PF_X (1 << 0)	       /* Segment is executable */
#define PF_W (1 << 1)	       /* Segment is writable */
#define PF_R (1 << 2)	       /* Segment is readable */
/* 处理器特定掩码 */
#define PF_MASKPROC 0xf0000000 /* Processor-specific */

#endif /* elf.h */
