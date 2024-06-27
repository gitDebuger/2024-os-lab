# Lab3实验报告

## 1. 思考题

### 1.1 Thinking 3.1

**结合MOS中的页目录自映射应用解释代码中 `e->env_pgdir[PDX(UVPT)] = PADDR(e->env_pgdir) | PTE_V` 的含义。**

以只读权限在 `UVPT` 映射它自己的页表。因此，用户程序可以通过 `UVPT` 读取它的页表。

这段代码的功能是填写进程页目录的自映射表项。

左侧展开后为 `e->env_pgdir[(UPVT>>PDSHIFT)&0x03FF]` 也就是获取自映射页目录项。自映射页目录项地址的计算公式如下：
$$
PDE\_self\_mapping=PD\_base+(PT\_base>>22<<2)
$$
这里的 `env_pgdir` 就相当于公式中的 `PD_base` 而 `UVPT` 就相当于公式中的 `PT_base` ，其余的相加以及左移操作已经由数组 `[]` 运算符完成。即将被赋值的就是自映射页目录项。

自映射页目录项由物理页号和标志位组成，通过 `PADDR` 我们得到了页目录的物理地址，也就是物理页号左移后的结果。然后通过或运算将权限 `PTE_V` 写入标志位。

所以这条语句的作用就是填写页目录自映射页目录项，包括物理页号或者物理地址，以及对应的只读权限。

### 1.2 Thinking 3.2

**`elf_load_seg` 以函数指针的形式，接受外部自定义的回调函数 `map_page` 。请你找到与之相关的 `data` 这一参数在此处的来源，并思考它的作用。没有这个参数可不可以？为什么？**

这里的 `data` 来自于调用者 `load_icode` 函数中的参数 `struct Env *e` ，这个参数是由 `env_create` 函数传递给 `load_icode` 函数的。在 `env_create` 函数中，我们通过调用 `env_alloc` 函数获取了一个空闲的进程控制块结构体，然后将这个进程控制块结构体传递给了 `load_icode` 函数。

在 `elf_load_seg` 中，我们循环将 `data` 传递给 `map_page` 以将数据加载到用户地址空间，这里的 `map_page` 来自于调用者 `load_icode` 函数，其值为 `load_icode_mapper` 函数。

如果没有这个参数，那么在 `elf_load_seg` 函数中就无法调用 `load_icode_mapper` 函数，就无法完成加载数据的工作。

### 1.3 Thinking 3.3

**结合 `elf_load_seg` 的参数和实现，考虑该函数需要处理哪些页面加载的情况。**

该函数主要分为三步：

* 第一步，将 `va` 前面与 `PAGE_SIZE` 不对齐的部分，按照原有偏移量加载到页面中。如果 `va` 已经与 `PAGE_SIZE` 对齐，则在该步骤中无加载操作。
* 第二步，将 `va` 后面的数据通过循环逐页加载到页面中。
* 第三步，如果数据大小 `bin_size` 小于段大小 `sgsize` ，则循环创建空页进行填充。

所以分支的来源包括 `va` 所处位置以及数据大小，具体类型如下：

1. `va` 与 `PAGE_SIZE` 未对齐
    1. `bin_size` 小于 `PAGE_SIZE-offset` 
        1. `bin_size` 小于 `sg_size` 
        2. `bin_size` 大于 `sg_size` 
    2. `bin_size` 大于 `PAGE_SIZE-offset`
        1. `bin_size` 小于 `sg_size` 
        2. `bin_size` 大于 `sg_size` 
2. `va` 与 `PAGE_SIZE` 对齐
    1. `bin_size` 小于 `sg_size` 
    2. `bin_size` 大于 `sg_size` 

共六种情况。

### 1.4 Thinking 3.4

**你认为这里的 `env_tf.cp0_epc` 存储的是物理地址还是虚拟地址?**

存储的应该是虚拟地址。

实验指导书中有提到：

> 在4Kc上，软件访存的虚拟地址会先被MMU硬件映射到物理地址，随后使用物理地址来访问内存或其他外设。

也就是说我们访存使用的所有地址都是虚拟地址。

### 1.5 Thinking 3.5

这些异常处理函数定义在 `kern/genex.S` 中。

其中 `handle_int` 函数直接定义，内容如下：

```assembly
NESTED(handle_int, TF_SIZE, zero)
	/* 首先读取CP0_CAUSE和CP0_STATUS寄存器的值 */
	mfc0    t0, CP0_CAUSE
	mfc0    t2, CP0_STATUS
	/* 然后将其与操作以确定是否有中断被触发 */
	and     t0, t2
	andi    t1, t0, STATUS_IM7
	/* 如果有中断被触发则跳转到 timer_irq 标签处处理定时器中断 */
	bnez    t1, timer_irq
timer_irq:
	li      a0, 0
	/* 最终调用 schedule 函数 */
	j       schedule
END(handle_int)
```

另外两个 `handle_mod` 和 `handle_tlb` 异常处理函数，利用宏进行定义。

首先定义了一个宏，用于生成异常处理函数：

```assembly
.macro BUILD_HANDLER exception handler
NESTED(handle_\exception, TF_SIZE + 8, zero)
	move    a0, sp
	addiu   sp, sp, -8
	jal     \handler
	addiu   sp, sp, 8
	j       ret_from_exception
END(handle_\exception)
.endm
```

然后调用这个宏，生成上述两个异常处理函数：

```assembly
BUILD_HANDLER tlb do_tlb_refill
BUILD_HANDLER mod do_tlb_mod
```

### 1.6 Thinking 3.6

**阅读 `entry.S` 、 `genex.S` 和 `env_asm.S` 这几个文件，并尝试说出时钟中断在哪些时候开启，在哪些时候关闭。**

时钟中断在调度执行每一个进程之前开启，当触发时钟中断后，进入中断处理程序。此时时钟中断已经被关闭，需要重置时钟状态，才能重新开启时钟中断。

### 1.7 Thinking 3.7

**阅读相关代码，思考操作系统是怎么根据时钟中断切换进程的。**

在操作系统中，设置了一个进程就绪队列，并且给每一个进程添加了一个时间片，当进程的时间片消耗完，会触发硬件的时钟中断，进入对应的中断处理程序。中断处理程序会保存现场，并将这个进程移动到就绪队列的尾端，复原其时间片。然后让就绪队列最首端的进程执行相应的时间片，按照这种规律实现循环往复，从而做到根据时钟周期切换进程。

## 2. 难点分析

***~~这次实验全是难点~~***

### 2.1 课下实验

这次课下实验相比于Lab2没有增加太多文件，但是有很多函数调用了Lab2内存管理的函数，调用关系比较复杂。

另外，进程管理涉及内核态与用户态的切换，这部分需要通过硬件完成，有一些汇编代码需要编写，理解难度较大。

### 2.2 Lab3-exam

***这部分需要实现一个返回进程运行信息的函数，看起来很难，其实一点也不简单。***

需要修改的文件较多，而且需要理解各个文件的调用关系。

**首先是 `kern/env.c` 中添加如下代码：**

```c
void env_stat(struct Env *e, u_int *pri, u_int *scheds, u_int *runs, u_int *clocks) {
	/* 进程优先级 */
	*pri = e->env_pri;
	/* 进程运行次数 */
	*runs = e->env_runs;
	/* 进程调度次数 */
	*scheds = e->env_ipc_value;
	/* 进程 CP0_COUNT 寄存器累加值 */
	*clocks = e->env_ipc_from;
}
```

这部分是函数的主体，为了让这个函数能获得正确的信息，还需要修改其他文件。

**对 `kern/sched.c` 中的函数做如下修改：**

```c
void schedule(int yield) {
	static int count = 0;
	struct Env *e = curenv;

	if (yield || count <= 0 || e == NULL || e->env_status != ENV_RUNNABLE) {
		if (e != NULL) {
			TAILQ_REMOVE(&env_sched_list, e, env_sched_link);
			if (e->env_status == ENV_RUNNABLE) {
				TAILQ_INSERT_TAIL(&env_sched_list, e, env_sched_link);
			}
		}
		if (TAILQ_EMPTY(&env_sched_list)) {
			panic("schedule: no runnable envs");
		}
		e = TAILQ_FIRST(&env_sched_list);
		/* Begin */
		/* 复用 env_ipc_value 为调度次数计数器 */
		/* 在发生进程调度时自增 1 */
		e->env_ipc_value++;
		/* End */
		count = e->env_pri;
	}
	count--;
	/* Begin */
	/* 复用 env_ipc_from 为 CP0_COUNT 寄存器在每次时钟中断时的累加值 */
	if (e->env_runs == 0) {
		/* 未运行过初始化为 0 */
		e->env_ipc_from = 0;
	} else {
		/* 此后每次调度累加 CP0_COUNT 寄存器的值 */
		e->env_ipc_from += ((struct Trapframe *)KSTACKTOP - 1)->cp0_clock;
	}
	/* End */
	env_run(e);
}
```

为了方便，这里我没有在进程控制块中添加新的字段，而是复用了两个关于进程通信的字段，因为本次实验不涉及进程通信。

**然后需要在 `include/trap.h` 中的 `struct Trapframe` 中添加如下字段：**

```c
unsigned long cp0_clock;
```

用于保存寄存器 `CP0_COUNT` 的值。

**并且在该文件的最后，添加并修改宏定义：**

```c
#define TF_CLOCK ((TF_EPC) + 4)
#define TF_SIZE ((TF_CLOCK) + 4)
```

**做完这些，在 `include/stackframe.h` 中添加如下指令，将 `CP0_COUNT` 寄存器的值存到陷入帧结构体中：**

```assembly
mfc0    k0, CP0_COUNT
sw      k0, TF_CLOCK(sp)
```

完成以上工作，这个实验代码就算完成了。

### 2.3 Lab3-extra

这个实验代码还是比较简单的，就是需要注意别忘了添加异常分发代码，还要修改异常向量组。

**首先在 `kern/genex.S` 中利用宏生成异常处理函数：**

```assembly
BUILD_HANDLER ri do_ri
```

**然后在 `kern/traps.c` 中添加一些代码：**

```c
#include <env.h>
#include <pmap.h>
#include <printk.h>
#include <trap.h>

extern void handle_int(void);
extern void handle_tlb(void);
extern void handle_sys(void);
extern void handle_mod(void);
extern void handle_reserved(void);
/* Begin Statement 1 */
extern void handle_ri(void);
/* End Statement 1 */

void (*exception_handlers[32])(void) = {
    [0 ... 31] = handle_reserved,
    [0] = handle_int,
    [2 ... 3] = handle_tlb,
	/* Begin Statement 2 */
    [10] = handle_ri,
	/* End Statement 2 */
#if !defined(LAB) || LAB >= 4
    [1] = handle_mod,
    [8] = handle_sys,
#endif
};

void do_reserved(struct Trapframe *tf) {
	print_tf(tf);
	panic("Unknown ExcCode %2d", (tf->cp0_cause >> 2) & 0x1f);
}

/* Begin Statement 3 */
void do_ri(struct Trapframe *tf) {
	u_int x = *((u_int *)(tf->cp0_epc));
	u_int high = x >> 26;
	u_int low = x & 0x3f;
	if (high != 0) {
		tf->cp0_epc += 4;
		return;
	} else if (low != 0x3f && low != 0x3e) {
		tf->cp0_epc += 4;
		return;
	} else if ((x >> 6 & 0x1f) != 0) {
		tf->cp0_epc += 4;
		return;
	}
	u_int rs, rt, rd;
	rs = x >> 21 & 0x1f;
	rt = x >> 16 & 0x1f;
	rd = x >> 11 & 0x1f;
	
	u_int nrs = tf->regs[rs];
	u_int nrt = tf->regs[rt];
	
	if (low == 0x3f) {
		u_int nrd = 0;
		for (int i = 0; i < 32; i += 8) {
			u_int rs_i = nrs & (0xff << i);
			u_int rt_i = nrt & (0xff << i);
			if (rs_i < rt_i) {
				nrd = nrd | rt_i;
			} else {
				nrd = nrd | rs_i;
			}
		}
		tf->regs[rd] = nrd;
	} else {
		u_int tmp = *((u_int *)nrs);
		if (tmp == nrt) {
			*((u_int *)nrs) = tf->regs[rd];
		}
		tf->regs[rd] = tmp;
	}

	tf->cp0_epc += 4;
}
/* End Statement 3 */
```

这部分总体来说还是可以的，如果计组有好好学的话。

## 3. 实验体会

随着实验的继续，涉及的文件越来越多，调用关系越来越复杂。一定要理清各个文件和各个函数之间的关系，才能做好每次实验。
