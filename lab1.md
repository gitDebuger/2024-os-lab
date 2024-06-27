# Lab1实验报告

## 思考题

### Thinking 1.1

**分别使用x86-64原生工具链和交叉编译工具链，重复编译和解析过程。**

首先创建一个 `example.c` 文件，内容如下：

```c
#include <stdio.h>
int main() {
    printf("Hello World!");
    return 0;
}
```

然后使用 `gcc` 将该文件编译为可执行文件：

```bash
gcc example.c -o example
```

在编译过程中 `gcc` 会自动调用 `ld` 链接器生成最终的可执行文件。

或者也可以手动链接，依次执行下面的命令：

```bash
gcc -c example.c -o example.o
ld -o example_ld example.o /usr/lib/x86_64-linux-gnu/crt1.o /usr/lib/x86_64-linux-gnu/crti.o /usr/lib/x86_64-linux-gnu/crtn.o -lc
```

上面的程序将标准运行时文件和标准库文件与 `example.o` 链接到一起。

工具 `ld` 的用法如下：

```bash
# 链接目标文件生成可执行文件
ld -o output example1.o example2.o
# 链接共享库生成可执行文件
ld -o output main.o -lmylib -L/path/to/mylib
# 生成共享库
ld -shared -o libmylib.so obj1.o obj2.o
```

不过不建议手动调用 `ld` 命令，通常使用 `gcc` 等编译器自动调用 `ld` 命令，手动使用 `ld` 命令会比较繁琐。

接下来使用 `objdump` 命令对 `example` 可执行文件和 `example.o` 目标文件进行反汇编：

```bash
objdump -DS example.o > example_o.dump
objdump -DS example > example.dump
```

可以发现，最终的可执行文件相比于可重定位目标文件，文件字节数增加，相应的地址也已经由 `0` 被替换为实际的地址。

命令 `objdump` 接收的参数如下：

```bash
-a, --archive-headers    # Display archive header information
-f, --file-headers       # Display the contents of the overall file header
-p, --private-headers    # Display object format specific file header contents
-P, --private=OPT,OPT... # Display object format specific contents
-h, --[section-]headers  # Display the contents of the section headers
-x, --all-headers        # Display the contents of all headers
-d, --disassemble        # Display assembler contents of executable sections
-D, --disassemble-all    # Display assembler contents of all sections
    --disassemble=<sym>  # Display assembler contents from <sym>
-S, --source             # Intermix source code with disassembly
    --source-comment[=<txt>] # Prefix lines of source code with <txt>
-s, --full-contents      # Display the full contents of all sections requested
-g, --debugging          # Display debug information in object file
-e, --debugging-tags     # Display debug information using ctags style
-G, --stabs              # Display (in raw form) any STABS info in the file
-W[lLiaprmfFsoRtUuTgAckK] or
--dwarf[=rawline,=decodedline,=info,=abbrev,=pubnames,=aranges,=macro,=frames,
        =frames-interp,=str,=loc,=Ranges,=pubtypes,
        =gdb_index,=trace_info,=trace_abbrev,=trace_aranges,
        =addr,=cu_index,=links,=follow-links]
                         # Display DWARF info in the file
--ctf=SECTION            # Display CTF info from SECTION
-t, --syms               # Display the contents of the symbol table(s)
-T, --dynamic-syms       # Display the contents of the dynamic symbol table
-r, --reloc              # Display the relocation entries in the file
-R, --dynamic-reloc      # Display the dynamic relocation entries in the file
@<file>                  # Read options from <file>
-v, --version            # Display this program's version number
-i, --info               # List object formats and architectures supported
-H, --help               # Display this information
```

所以上面的示例中的 `-DS` 意为显示所有节的汇编内容，并且将源码和汇编码混合对比显示。

接下来使用 `readelf` 命令对可执行文件和可重定位目标文件的ELF进行分析，该命令的参数如下：

```bash
-a --all               Equivalent to: -h -l -S -s -r -d -V -A -I
-h --file-header       Display the ELF file header
-l --program-headers   Display the program headers
   --segments          An alias for --program-headers
-S --section-headers   Display the sections' header
   --sections          An alias for --section-headers
-g --section-groups    Display the section groups
-t --section-details   Display the section details
-e --headers           Equivalent to: -h -l -S
-s --syms              Display the symbol table
   --symbols           An alias for --syms
--dyn-syms             Display the dynamic symbol table
-n --notes             Display the core notes (if present)
-r --relocs            Display the relocations (if present)
-u --unwind            Display the unwind info (if present)
-d --dynamic           Display the dynamic section (if present)
-V --version-info      Display the version sections (if present)
-A --arch-specific     Display architecture specific information (if any)
-c --archive-index     Display the symbol/file index in an archive
-D --use-dynamic       Use the dynamic section info when displaying symbols
-x --hex-dump=<number|name>
                       Dump the contents of section <number|name> as bytes
-p --string-dump=<number|name>
                       Dump the contents of section <number|name> as strings
-R --relocated-dump=<number|name>
                       Dump the contents of section <number|name> as relocated bytes
-z --decompress        Decompress section before dumping it
-w[lLiaprmfFsoRtUuTgAckK] or
--debug-dump[=rawline,=decodedline,=info,=abbrev,=pubnames,=aranges,=macro,=frames,
             =frames-interp,=str,=loc,=Ranges,=pubtypes,
             =gdb_index,=trace_info,=trace_abbrev,=trace_aranges,
             =addr,=cu_index,=links,=follow-links]
                       Display the contents of DWARF debug sections
--dwarf-depth=N        Do not display DIEs at depth N or greater
--dwarf-start=N        Display DIEs starting with N, at the same depth
                       or deeper
--ctf=<number|name>    Display CTF info from section <number|name>
--ctf-parent=<number|name>
                       Use section <number|name> as the CTF parent
--ctf-symbols=<number|name>
                       Use section <number|name> as the CTF external symtab
--ctf-strings=<number|name>
                       Use section <number|name> as the CTF external strtab
-I --histogram         Display histogram of bucket list lengths
-W --wide              Allow output width to exceed 80 characters
@<file>                Read options from <file>
-H --help              Display this information
-v --version           Display the version number of readelf
```

这里使用 `-a` 来输出所有ELF信息：

```bash
readelf -a example
readelf -a example.o
```

接下来使用 `mips-linux-gnu-gcc` 交叉编译工具链，依次执行如下命令：

```bash
mips-linux-gnu-gcc example.c -o example_cross
mips-linux-gnu-gcc -c example.c -o example_cross.o
readelf -a example_cross.o
readelf -a example_cross
```

这样就完成了使用交叉编译工具链编译和解析的工作。

### Thinking 1.2

**使用手动编写的 `readelf` 程序解析 `target/mos` 内核程序。**

在 `tools/readelf` 路径下执行如下命令：

```bash
./readelf ../../target/mos
```

在本机的运行结果为：

```
0:0x0
1:0x80020000
2:0x80021920
3:0x80021938
4:0x80021950
5:0x0
6:0x0
7:0x0
8:0x0
9:0x0
10:0x0
11:0x0
12:0x0
13:0x0
14:0x0
15:0x0
16:0x0
17:0x0
```

**为什么我们编写的 `readelf` 程序不能解析 `readelf` 文件本身而系统工具 `readelf` 则可以解析？**

在编写 `readelf.c` 时可以发现，我们使用的结构体中的变量均为三十二位变量：

```c
typedef struct {
        unsigned char e_ident[EI_NIDENT]; /* Magic number and other info */
        Elf32_Half e_type;                /* Object file type */
        Elf32_Half e_machine;             /* Architecture */
        Elf32_Word e_version;             /* Object file version */
        Elf32_Addr e_entry;               /* Entry point virtual address */
        Elf32_Off e_phoff;                /* Program header table file offset */
        Elf32_Off e_shoff;                /* Section header table file offset */
        Elf32_Word e_flags;               /* Processor-specific flags */
        Elf32_Half e_ehsize;              /* ELF header size in bytes */
        Elf32_Half e_phentsize;           /* Program header table entry size */
        Elf32_Half e_phnum;               /* Program header table entry count */
        Elf32_Half e_shentsize;           /* Section header table entry size */
        Elf32_Half e_shnum;               /* Section header table entry count */
        Elf32_Half e_shstrndx;            /* Section header string table index */
} Elf32_Ehdr;

typedef struct {
        Elf32_Word p_type;   /* Segment type */
        Elf32_Off p_offset;  /* Segment file offset */
        Elf32_Addr p_vaddr;  /* Segment virtual address */
        Elf32_Addr p_paddr;  /* Segment physical address */
        Elf32_Word p_filesz; /* Segment size in file */
        Elf32_Word p_memsz;  /* Segment size in memory */
        Elf32_Word p_flags;  /* Segment flags */
        Elf32_Word p_align;  /* Segment alignment */
} Elf32_Phdr;
```

并且该程序没有针对三十二位ELF文件和六十四位ELF文件进行分支处理，所以该程序只能解析三十二位ELF文件，而 `readelf` 可执行程序是一个六十四位ELF文件，自然无法解析。

### Thinking 1.3

**实验操作系统的内核入口并没有放在上电启动地址，而是按照内存布局图放置。**

**为什么这样放置内核还能保证内核入口被正确跳转到？**

操作系统的启动通常可以分为两个阶段，分别是引导阶段和加载阶段。

引导阶段主要任务是加载引导程序到内存中，并开始执行引导程序。加载阶段的任务是，引导加载程序加载操作系统内核程序到内存中的预定位置，然后跳转到内核程序的入口点，并开始执行内核代码。

实验操作系统采用了QEMU模拟器，由于QEMU已经提供了引导功能，不再需要实现引导程序，启动流程被简化为加载内核到内存，然后跳转到内核程序的入口。

所以在实验中，直接按照内存布局放置操作系统内核入口即可，因为QEMU模拟器会负责跳转到内核入口，不需要放在上电启动地址。

## 难点分析

### 整体项目结构

整个实验项目文件众多，读懂每个文件，理解整个项目的结构并不容易。

仅仅课程提供的模板代码及其中的注释，还有实验指导书，不足以完全理解整个项目的体系结构，还需要通过ChatGPT、GitHub和知乎等平台，查询相关的资料。读懂项目代码这一步需要主动查找资料的意识和能力，考验了学生的主观能动性。

项目文件中主要包含 `.c` 源文件、汇编语言 `.S` 文件和 `Makefile` 文件，三种语言混合，各个文件互相引用，理清项目体系结构绝非易事。

### ELF文件分析工具

在编写 `readelf` 时，需要理解ELF文件的结构，并对 `Elf32_Ehdr` 结构体和 `Elf32_Shdr` 结构体完全理解，才能够正确编写出 `readelf.c` 文件的核心代码。

### 输入输出函数

输出功能使用的定义如下：

```c
typedef void (*fmt_callback_t)(void *data, const char *buf, size_t len);
void vprintfmt(fmt_callback_t out, void *data, const char *fmt, va_list ap);
void outputk(void *data, const char *buf, size_t len);
void printk(const char *fmt, ...);
void printcharc(char ch);
```

几个函数依次调用，需要理解这个结构，才能够完成对限时测试输入功能的编写。

输入功能使用的定义如下：

```c
typedef void (*scan_callback_t)(void *data, char *buf, size_t len);
int vscanfmt(scan_callback_t in, void *data, const char *fmt, va_list ap);
void inputk(void *data, char *buf, size_t len);
int scanf(const char *fmt, ...);
int scancharc(void);
```

需要理解输出功能的实现，考验了理解转化的能力。

## 实验体会

本次实验是整个操作系统实验的开始，此时的文件量还不是很大，做好本次实验，理解项目体系结构，才能顺利完成后续的操作系统实验。综合来看，本次实验给整个操作系统实验开了个好头。

