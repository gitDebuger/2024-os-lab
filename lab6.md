# Lab6实验报告

## 1. 思考题

### 1.1 Thinking 6.1

**示例代码中，父进程操作管道的写端，子进程操作管道的读端。如果现在想让父进程作为“读者”，代码应当如何修改？**

```c
#include <stdlib.h>
#include <unistd.h>

int fildes[2];
/* buf size is 100 */
char buf[100];
int status;
int main() {
    status = pipe(fildes);
    if (status == -1) {
        /* an error occurred */
        printf("error\n");
    }
    switch (fork()) {
        case -1: /* Handle error */
            break;
        case 0: /* Child - writes to pipe */
            close(fildes[0]); /* Read end is unused */
            write(fildes[1], "Hello world\n", 12); /* Write data on pipe */
            close(fildes[1]); /* Child will see EOF */
            exit(EXIT_SUCCESS);
        default: /* Parent - reads from pipe */
            close(fildes[1]); /* Write end is unused */
            read(fildes[0], buf, 100); /* Get data from pipe */
            printf("parent-process read:%s", buf); /* Print the data */
            close(fildes[0]); /* Finished with pipe */
            exit(EXIT_SUCCESS);
    }
}
```

也就是交换 `case 0` 和 `case 1` 处的代码。

### 1.2 Thinking 6.2

**上面这种不同步修改 `pp_ref` 而导致的进程竞争问题在 `user/lib/fd.c` 中的 `dup` 函数中也存在。请结合代码模仿上述情景，分析一下我们的 `dup` 函数中为什么会出现预想之外的情况？**

`dup` 函数的功能是将文件描述符 `oldfdnum` 所对应的内容映射到文件描述符 `newfdnum` 中，会将 `oldfdnum` 和 `pipe` 的引用次数都增加 `1` ，将 `newfdnum` 的引用次数变为 `oldfdnum` 的引用次数。

当我们将一个管道的读/写端对应的文件描述符映射到另一个文件描述符。在进行映射之前，`f[0]` `f[1]` `pipe` 的引用次数分别为 `1` `1` `2` 。按照 `dup` 函数的执行顺序，会先将 `fd[0]` 引用次数 `+1` ，再将 `pipe` 引用次数 `+1` ，如果 `fd[0]` 的引用次数 `+1` 后恰好发生了一次时钟中断，进程切换后，另一进程调用 `_pipeisclosed` 函数判断管道写端是否关闭，此时 `pageref(fd[0]) == pageref(pipe) == 2` ，所以会误认为写端关闭，从而出现判断错误。

### 1.3 Thinking 6.3

**阅读上述材料并思考：为什么系统调用一定是原子操作呢？如果你觉得不是所有的系统调用都是原子操作，请给出反例。希望能结合相关代码进行分析说明。**

不只是系统调用，所有的异常处理函数都是原子操作。

```c
/* 定义通用异常入口点 */
.section .text.exc_gen_entry
exc_gen_entry:
	/* 首先保存了所有寄存器的状态 */
	SAVE_ALL
	/* 重置了处理器的状态 */
	/* 确保处理器处于内核模式并禁用了中断 */
	/* 清除了状态寄存器中的用户模式 UM 位 */
	/* 异常级别 EXL 位和全局中断使能 IE 位 */
	mfc0    t0, CP0_STATUS
	and     t0, t0, ~(STATUS_UM | STATUS_EXL | STATUS_IE)
	mtc0    t0, CP0_STATUS
	/* 获取异常原因并跳转到相应的异常处理函数 */
	mfc0    t0, CP0_CAUSE
	andi    t0, 0x7c
	lw      t0, exception_handlers(t0)
	jr      t0
```

这是 `kern/entry.S` 中定义的通用异常入口点，在这里会禁用中断，从而避免再次触发异常处理，从而确保当前异常处理函数为原子操作。从而，所有的系统调用都是原子操作。

### 1.4 Thinking 6.4

**仔细阅读上面这段话，并思考下列问题：**

* **按照上述说法控制 `pipe_close` 中 `fd` 和 `pipe_unmap` 的顺序，是否可以解决上述场景的进程竞争问题？给出你的分析过程。**
* **我们只分析了 `close` 时的情形，在 `fd.c` 中有一个 `dup` 函数，用于复制文件描述符。试想，如果要复制的文件描述符指向一个管道，那么是否会出现与 `close` 类似的问题？请模仿上述材料写写你的理解。**

回答如下：

* 可以解决。如果程序正常运行，那么 `pipe` 的 `pageref` 是要大于 `fd` 的，在执行 `unmap` 操作时，优先解除 `fd` 的映射，这样就可保证严格大于关系恒成立，即使发生了时钟中断，也不会出现运行错误。
* 同样的道理，在 `dup` 使引用次数增加时，先将 `pipe` 的引用次数增加，保证不会出现两者相等的情况。

### 1.5 Thinking 6.5

**思考以下三个问题。**

* **认真回看Lab5文件系统相关代码，弄清打开文件的过程。**
* **回顾Lab1与 Lab3，思考如何读取并加载 `ELF` 文件。**
* **在Lab1中我们介绍了 `data` `text` `bss` 段及它们的含义，`data` 段存放初始化过的全局变量，`bss` 段存放未初始化的全局变量。关于 `memsize` 和 `filesize` ，我们在Note 1.3.4中也解释了它们的含义与特点。关于Note 1.3.4，注意其中关于“ `bss` 段并不在文件中占数据”表述的含义。回顾Lab3并思考：`elf_load_seg()` 和 `load_icode_mapper()` 函数是如何确保加载 `ELF` 文件时，`bss` 段数据被正确加载进虚拟内存空间。`bss` 段在 `ELF` 中并不占空间，但 `ELF` 加载进内存后，`bss` 段的数据占据了空间，并且初始值都是 `0` 。请回顾 `elf_load_seg()` 和 `load_icode_mapper()` 的实现，思考这一点是如何实现的？**

回答如下：

* 用户进程通过 `user/lib/file.c` 中的 `open` 函数完成打开文件操作。在该函数中，首先调用 `fd_alloc` 申请一个文件描述符，但是这里只是得到了可以作为文件描述符的地址，还没有实际的文件描述符数据。之后调用 `fsipc_open` ，该函数会将 `path` 对应的文件以 `mode` 的方式打开，将该文件的文件描述符共享到 `fd` 指针对应的地址处。

    虽然获得了服务进程共享给用户进程的文件描述符，可文件的内容还没有被一同共享过来，接下来还需要使用 `fsipc_map` 进行映射。先做准备工作，通过 `fd2data` 获取文件内容应该映射到的地址；接着将文件所有的内容都从磁盘中映射到内存。

    在最后，使用 `fd2num` 方法获取文件描述符在文件描述符数组中的索引，并将其返回给调用者。

    ```c
    int open(const char *path, int mode) {
    	int r;
    	struct Fd *fd;
        // 申请文件描述符
    	try(fd_alloc(&fd));
        // 打开文件并将文件描述符共享到 fd 指针对应地址处
    	try(fsipc_open(path, mode, fd));
        // 映射文件内容
    	char *va;
    	struct Filefd *ffd;
    	u_int size, fileid;
    	va = fd2data(fd);
    	ffd = (struct Filefd *)fd;
    	size = ffd->f_file.f_size;
    	fileid = ffd->f_fileid;
    	for (int i = 0; i < size; i += PTMAP) {
    		try(fsipc_map(fileid, i, va + i));
    	}
        // 返回索引
    	return fd2num(fd);
    }
    ```

    除此之外，还可以在 `mips_init` 函数中在操作系统初始化时创建初始进程，此时直接从宿主机中读取文件。

* 加载 `ELF` 文件主要是通过 `lib/elfloader.c` 中的 `elf_load_seg` 函数。

    ```c
    int elf_load_seg(Elf32_Phdr *ph, const void *bin, elf_mapper_t map_page, void *data) {
    	u_long va = ph->p_vaddr;
    	size_t bin_size = ph->p_filesz;
    	size_t sgsize = ph->p_memsz;
    	u_int perm = PTE_V;
    	if (ph->p_flags & PF_W) {
    		perm |= PTE_D;
    	}
    	/* 首先需要处理要加载的虚拟地址不与页对齐的情况 */
    	/* 将最开头不对齐的部分剪切下来先映射到内存的页中 */
    	int r;
    	size_t i;
    	u_long offset = va - ROUNDDOWN(va, PAGE_SIZE);
    	if (offset != 0) {
    		if ((r = map_page(data, va, offset, perm, bin,
    				  MIN(bin_size, PAGE_SIZE - offset))) != 0) {
    			return r;
    		}
    	}
    	/* 接着处理数据后面的部分 */
    	/* 通过循环不断将数据加载到页上。 */
    	for (i = offset ? MIN(bin_size, PAGE_SIZE - offset) : 0; i < bin_size; i += PAGE_SIZE) {
    		if ((r = map_page(data, va + i, 0, perm, bin + i, MIN(bin_size - i, PAGE_SIZE))) !=
    		    0) {
    			return r;
    		}
    	}
    	/* 最后我们处理段大小大于数据大小的情况 */
    	/* 在这一部分我们不断创建新的页 */
    	/* 但是并不向其中加载任何内容 */
    	while (i < sgsize) {
    		if ((r = map_page(data, va + i, 0, perm, NULL, MIN(sgsize - i, PAGE_SIZE))) != 0) {
    			return r;
    		}
    		i += PAGE_SIZE;
    	}
    	return 0;
    }
    ```

    在调用该函数加载之前，还需要调用同文件中的 `elf_from` 函数检查要加载的文件是否为 `ELF` 文件。

    ```c
    const Elf32_Ehdr *elf_from(const void *binary, size_t size) {
    	const Elf32_Ehdr *ehdr = (const Elf32_Ehdr *)binary;
    	if (size >= sizeof(Elf32_Ehdr) && ehdr->e_ident[EI_MAG0] == ELFMAG0 &&
    	    ehdr->e_ident[EI_MAG1] == ELFMAG1 && ehdr->e_ident[EI_MAG2] == ELFMAG2 &&
    	    ehdr->e_ident[EI_MAG3] == ELFMAG3 && ehdr->e_type == 2) {
    		return ehdr;
    	}
    	return NULL;
    }
    ```

    这两个函数的调用者为 `kern/env.c` 中的 `load_icode` 函数。

* 处理 `bss` 段的函数是Lab3中的 `load_icode_mapper` 。在这个函数中，我们要对 `bss` 段进行内存分配，但不进行初始化。当 `bin_size < sgsize` 时，会将空位填 `0` ，在这段过程中为 `bss` 段的数据全部赋上了默认值 `0` 。

### 1.6 Thinking 6.6

**通过阅读代码空白段的注释我们知道，将标准输入或输出定向到文件，需要我们将其 `dup` 到 `0` 或 `1` 号文件描述符。那么问题来了：在哪步 `0` 和 `1` 被安排为标准输入和标准输出？请分析代码执行流程，给出答案。**

在 `user/init.c` 中的 `main` 函数中有如下代码：

```c
// other codes ...
// stdin should be 0, because no file descriptors are open yet
if ((r = opencons()) != 0) {
	user_panic("opencons: %d", r);
}
// stdout
if ((r = dup(0, 1)) < 0) {
	user_panic("dup: %d", r);
}
// other codes ...
```

### 1.7 Thinking 6.7

**在 `shell` 中执行的命令分为内置命令和外部命令。在执行内置命令时 `shell` 不需要 `fork` 一个子 `shell` ，如Linux系统中的 `cd` 命令。在执行外部命令时 `shell` 需要 `fork` 一个子 `shell` ，然后子 `shell` 去执行这条命令。**

**据此判断，在MOS中我们用到的 `shell` 命令是内置命令还是外部命令？请思考为什么Linux的 `cd` 命令是内部命令而不是外部命令？**

在MOS中，我们用到的 `shell` 命令是外部命令，需要 `fork` 一个子 `shell` 来执行命令。

Linux的 `cd` 指令是改变当前的工作目录，如果在子 `shell` 中执行，则改变的是子 `shell` 的工作目录，无法改变当前 `shell` 的工作目录。

### 1.8 Thinking 6.8

**在你的 `shell` 中输入命令 `ls.b | cat.b > motd` 。**

**请问你可以在你的 `shell` 中观察到几次 `spawn` ？分别对应哪个进程？**

**请问你可以在你的 `shell` 中观察到几次进程销毁？分别对应哪个进程？**

执行结果如下：

```yaml
$ ls.b | cat.b > motd
[00004003] pipecreate
[00005005] destroying 00005005
[00005005] free env 00005005
i am killed ...
[00005806] destroying 00005806
[00005806] free env 00005806
i am killed ...
[00004804] destroying 00004804
[00004804] free env 00004804
i am killed ...
[00004003] destroying 00004003
[00004003] free env 00004003
i am killed ...
```

这里没有观察到 `spwan` ，进程销毁次数为 `4` 。

## 2. 难点分析

这部分实验的核心在于 `user/lib/pipe.c` 中与管道相关的函数。

首先是 `_pipe_is_closed` 函数，其中需要使用一个循环，不断进行 `env_runs` 的判断，直到稳定为止。

```c
do {
    runs = env->env_runs;

    fd_ref = pageref(fd);
    pipe_ref = pageref(p);

} while (runs != env->env_runs);
```

然后是 `pipe_read` 函数，在 `p_rpos >= p_wpos` 时不能立刻返回，而是应该根据 `_pipe_is_closed()` 的返回值判断管道是否关闭，若未关闭，应执行进程切换。

```c
while (p->p_rpos >= p->p_wpos) {
    if (i > 0 || _pipe_is_closed(fd, p)) {
        return i;
    } else {
        syscall_yield();
    }
}
```

最后是 `pipe_write` 函数，同理在 `p_wpos - p_rpos >= PIPE_SIZE` 时不能立刻返回，而是应该根据 `_pipe_is_closed()` 的返回值判断管道是否关闭，若未关闭，应执行进程切换。

```c
while (p->p_wpos - p->p_rpos >= PIPE_SIZE) {
    if (_pipe_is_closed(fd, p)) {
        return i;
    } else {
        syscall_yield();
    }
}
```

## 3. 实验体会

这就是整个操作系统实验课的最后一部分了，~~我们成功体验到了从头搭建一个小操作系统的乐趣，~~至此，我们的操作系统虽然简单但是也还算完善，可以完成操作系统应该能够完成的大部分工作了。

在写下这篇实验报告时，操作系统理论课已经结束，实验课也就还有最后一次限时上机，期末考试也即将到来，我们真的要和操作系统课程说再见了。

回顾这一学期的操作系统课程，虽然内容很多，并且学得很累，但其实也还算有收获吧。不管怎么说吧，完结撒花！