# Lab4实验报告

## 1. 思考题

### 1.1 Thinking 4.1

**思考并回答下面的问题：**

* **内核在保存现场的时候是如何避免破坏通用寄存器的？**

    在系统调用异常处理函数 `andle_sys` 的开始，调用了 `SAVE_ALL` 宏， `SAVE_ALL` 将把所有的寄存器保存到指定位置，进而保证了在后续步骤不会破坏通用寄存器。在系统调用结束， `handle_sys` 调用 `j ret_from_exception` ，在这里，调用了 `RESTORE_SOME` 将所有通用寄存器的值恢复。

* **系统陷入内核调用后可以直接从当时的 `$a0-$a3` 参数寄存器中得到用户调用 `msyscall` 留下的信息吗？**

    可以，因为 `msyscall` 函数调用时，寄存器 `$a0-$a3` 用于存放前四个参数。执行 `syscall` 并没有改变这四个寄存器。

* **我们是怎么做到让 `sys` 开头的函数“认为”我们提供了和用户调用 `msyscall` 时同样的参数的？**

    在 `handle_sys` 中先取出 `$a0` 到 `$a3` ，再从用户栈中取出其他的参数，最后将这些参数保存到内核栈中，模拟使得内核态的 `sys` 函数可以正常将这些参数传入到函数中。在跳转到 `sys` 开头的函数之前， `handle_sys` 将需要传递的参数都存到了栈中。在 `sys` 开头的函数中，会从栈中找传递的参数，就“认为”我们提供了和用户调用 `msyscall` 时同样的参数。

* **内核处理系统调用的过程对 `Trapframe` 做了哪些更改？这种修改对应的用户态的变化是？**

    `handle_sys` 函数在把上下文环境保存到 `Trapframe` 中后，取出 `EPC` 并将 `EPC` 加 `4` ，在返回用户态后，从 `syscall` 的后一条指令开始执行。将返回值存入 `$v0` 寄存器，用户态可以正常获得系统调用的返回值。

### 1.2 Thinking 4.2

**思考 `envid2env` 函数：为什么 `envid2env` 需要判断 `e->env_id != envid` 的情况？如果没有这步判断会发生什么情况？**

这实际上考虑了这样一种情况，某一进程完成运行，资源被回收，这时其对应的进程控制块会插入回 `env_free_list` 中。当我们需要再次创建内存时，就可能重新取得该进程控制块，并为其赋予不同的 `envid` 。这时，已销毁进程的 `envid` 和新创建进程的 `envid` 都能通过 `ENVX` 宏取得相同的值，对应了同一个进程控制块。可是已销毁进程的 `envid` 却不应当再次出现，表达式 `e->env_id != envid` 就处理了 `envid` 属于已销毁进程的情况。

### 1.3 Thinking 4.3

**思考下面的问题，并对这个问题谈谈你的理解：请回顾 `lib/env.c` 文件中 `mkenvid()` 函数的实现，该函数不会返回 `0` ，请结合系统调用和 `IPC` 部分的实现与 `envid2env()` 函数的行为进行解释。**

首先，函数 `mkenvid` 的实现如下：

```c
u_int mkenvid(struct Env *e) {
	static u_int i = 0;
	return ((++i) << (1 + LOG2NENV)) | (e - envs);
}
```

该函数内部定义了自增的静态变量 `i` 用于确保生成的 `envid` 的唯一性，每次左移 `11` 位再或上 `e-envs` 也就是当前进程控制块在进程控制块数组中的偏移量。该函数第一次执行时返回的值为 `1<<11|(e-envs)` ，因此该函数不会返回 `0` 。

函数 `envid2env` 的实现如下：

```c
int envid2env(u_int envid, struct Env **penv, int checkperm) {
	struct Env *e;
	
	/* Exercise 4.3: Your code here. (1/2) */
	if (envid == 0) {
		*penv = curenv;
		return 0;
	} else {
		e = envs + ENVX(envid);
	}

	if (e->env_status == ENV_FREE || e->env_id != envid) {
		return -E_BAD_ENV;
	}
	/* Exercise 4.3: Your code here. (2/2) */
	if (checkperm && e->env_id != curenv->env_id && e->env_parent_id != curenv->env_id) {
		return -E_BAD_ENV;
	}

	*penv = e;
	return 0;
}
```

可以看出，如果传入的 `envid` 是 `0` ，那么直接返回 `curenv` 。因此上述 `envid` 为 `0` 的进程无法通过进程号被找到。

在 `IPC` 中如果要发送消息，需要通过 `envid` 找到对应进程，而通过 `envid2env` 找到的是当前进程而不一定是想要发送到的 `envid` 为 0 的进程，从而造成消息错误发送，接受方也无法收到对应的消息。

此外，在 `fork` 函数中，父进程的返回值为子进程的 `envid` ，子进程的返回值为 `0` ，如果存在进程号为 `0` 的进程，系统很可能会把一个父进程误认为是子进程，从而执行错误的操作。

结合这两个案例，就可以解释为什么 `mkenvid` 函数不能返回 `0` ，根本原因是 `0` 有特殊的含义，不能作为进程号存在。

### 1.4 Thinking 4.4

**关于 `fork` 函数的两个返回值，下面说法正确的是：**

**A. `fork` 在父进程中被调用两次，产生两个返回值。**

**B. `fork` 在两个进程中分别被调用一次，产生两个不同的返回值**

**C. `fork` 只在父进程中被调用了一次，在两个进程中各产生一个返回值**

**D. `fork` 只在子进程中被调用了一次，在两个进程中各产生一个返回值**

正确的选项是 C 选项。经典的一次调用、两次返回，在父进程中被调用一次，在两个进程中各产生一个返回值。子进程在父进程调用 `fork` 时被创建，并赋予不同返回值，子进程返回值为 `0` ，父进程返回值为子进程的进程号。

### 1.5 Thinking 4.5

**我们并不应该对所有的用户空间页都使用 `duppage` 进行映射。那么究竟哪些用户空间页应该映射，哪些不应该呢？请结合 `kern/env.c` 中 `env_init` 函数进行的页面映射和 `include/mmu.h` 里的内存布局图以及本章的后续描述进行思考。**

从内存布局图来看，我们需要保护的用户空间页为 `UTEXT` 到 `USTACKTOP` 的这一段，因为从 `USTACKTOP` 再往上到 `UXSTACKTOP` 这一段属于异常栈和无效内存的范围。

- 每个进程的异常处理栈属于自己的，不能映射给子进程。如果允许写时复制，则会导致：进程异常栈被写 -> 触发写时复制缺⻚异常 -> 需要保存现场 -> 写进程异常栈 -> 触发写时复制缺⻚异常 -> 死循环。
- 无效内存的范围显然不需要被保护。

与此同时，在 `UTEXT` 到 `USTACKTOP` 这一段中，也并不是所有页都要被保护。

- 首先，只读的页不需要被保护。
- 其次，用 `PTE_LIBRARY` 标识的页为共享页，同样不需要被保护。
- 其他的页无论是否已含有 `PTE_COW` ，都要用 `PTE_COW` 标记以作为保护。

### 1.6 Thinking 4.6

**在遍历地址空间存取页表项时你需要使用到 `vpd` 和 `vpt` 这两个指针，请参考 `user/include/lib.h` 中的相关定义，思考并回答这几个问题：**

* **`vpt` 和 `vpd` 的作用是什么？怎样使用它们？**

    这两个指针的定义如下：

    ```c
    #define vpt ((const volatile Pte *)UVPT)
    #define vpd ((const volatile Pde *)(UVPT + (PDX(UVPT) << PGSHIFT)))
    ```

    其中 `UVPT` 定义在 `include/mmu.h` 中

    ```c
    #define UVPT (ULIM - PDMAP)
    ```

    结合内存布局图，该宏意为页表的起始地址。

    根据页目录的自映射机制，这两个指针中 `vpt` 是指向二级页表的指针，而 `vpd` 是指向一级页表，也即页目录的指针。

    对于虚拟地址 `va` ，使用 `vpd[va >> 22]` 可以得到二级页表的物理地址，使用 `vpt[va >> 12]` 可以得到 `va` 对应的物理页面。

* **从实现的角度谈一下为什么进程能够通过这种方式来存取自身的页表？**

    `vpt` 与 `vpd` 本质上是通过宏定义的方式来对用户态的一段内存地址进行映射，因此使用这种方式实际上就是在使用内存布局图中的地址，所以可以通过这种方式来存取进程自身页表。

* **它们是如何体现自映射设计的？**

    `vpd` 本身处于 `vpt` 段中，说明页目录本身处于其所映射的页表中的一个页面里面。所以这两个指针的设计中运用了自映射。

* **进程能够通过这种方式来修改自己的页表项吗？**

    不能。进程本身处于用户态，不可以修改自身页表项，这两个指针仅供页表和页目录访问所用。这部分地址只读，用户不能修改。

### 1.7 Thinking 4.7

**在 `do_tlb_mod` 函数中，你可能注意到了一个向异常处理栈复制 `Trapframe` 运行现场的过程，请思考并回答这几个问题：**

* **这里实现了一个支持类似于“异常重入”的机制，而在什么时候会出现这种“异常重入”？**

    在用户发生写时复制引发的缺页中断并进行处理时，可能会再次发生缺页中断，从而出现“中断重入”。

* **内核为什么需要将异常的现场 `Trapframe` 复制到用户空间？**

    在微内核结构中，对缺页错误的处理由用户进程完成，用户进程在处理过程中需要读取 `Trapframe` 的内容；同时，在处理结束后同样是由用户进程恢复现场，会用到 `Trapframe` 中的数据。

### 1.8 Thinking 4.8

**在用户态处理页写入异常，相比于在内核态处理有什么优势？**

陷入内核会增添操作系统内核的工作量；且让用户进程实现内核功能体现了微内核思想，全方位保证操作系统正常运行。将异常处理交给用户进程，可以让内核做更多其他的事情。

### 1.9 Thinking 4.9

**请思考并回答以下几个问题：**

* **为什么需要将 `syscall_set_tlb_mod_entry` 的调用放置在 `syscall_exofork` 之前？**

    在调用 `syscall_env_alloc` 的过程中也可能需要进行异常处理，在调用 `fork` 时，有可能当前进程已是之前进程的子进程，从而需要考虑是否会发生写时复制的缺页中断异常处理，如果这时还没有调用过 `syscall_set_tlb_mod_entry` 则无法处理异常。

* **如果放置在写时复制保护机制完成之后会有怎样的效果？**

    在 `fork` 函数中，会先后调用 `syscall_set_tlb_mod_entry` 函数和 `duppage` 函数，在 `duppage` 函数中进行写时复制保护机制的设置。而在这两个函数之间会调用 `syscall_exofork` 创建子进程，创建的过程中会将父进程的陷入帧复制给子进程，这其中就应该包含 `syscall_set_tlb_mod_entry` 函数设置的进程控制块中的 `env_user_tlb_mod_entry` 字段。如果在写时复制保护机制设置完成之后才调用 `syscall_set_tlb_mod_entry` 函数，那么前面的字段将不会设置给子进程，或者说需要在父进程和子进程各执行一遍 `syscall_set_tlb_mod_entry` 调用才能完成设置。

## 2. 难点分析

这部分实验主要分为三部分：系统调用、进程通信和 `fork` 机制。

课下实验大家一定都是“借鉴”往年学长的作品，这里就不再赘述了，这里主要想分析一下限时上机的代码。

### 2.1 1-exam

这部分主要是实现一个信号量机制，包括信号量的打开、关闭和PV操作。其中有意思的一点是，信号量可以被重复打开，但是后续的打开操作不再重新初始化信号量。

```c
void sys_sem_open(int sem_id, int n) {
	// Lab 4-1-Exam: Your code here. (6/9)
	if (sem_id >= 0 && sem_id < 15 && !sems_valid[sem_id]) {
		sems_valid[sem_id] = 1;
		sems[sem_id] = n;
	}
}
```

### 2.2 1-extra

这部分是实现一个消息队列，主要实现思路就是将 `sys_ipc_try_send` 的操作拆分成两个函数，分别是 `sys_msg_send` 和 `sys_msg_recv` 函数。将相应的操作存放在消息控制块中，相当于一种延时执行。

```c
int sys_msg_send(u_int envid, u_int value, u_int srcva, u_int perm) {
	struct Env *e;
	struct Page *p;
	struct Msg *m;

	if (srcva != 0 && is_illegal_va(srcva)) {
		return -E_INVAL;
	}
	try(envid2env(envid, &e, 0));
	if (TAILQ_EMPTY(&msg_free_list)) {
		return -E_NO_MSG;
	}

	/* Your Code Here (1/3) */
	m = TAILQ_FIRST(&msg_free_list);
	m->msg_tier++;
	m->msg_status = MSG_SENT;
	TAILQ_REMOVE(&msg_free_list, m, msg_link);
	m->msg_from = curenv->env_id;
	m->msg_value = value;
	m->msg_perm = PTE_V | perm;
	if (srcva != 0) {
		p = page_lookup(curenv->env_pgdir, srcva, NULL);
		if (p == NULL) {
			return -E_INVAL;
		}
		p->pp_ref++;
		m->msg_page = p;
	} else {
		m->msg_page = NULL;
	}
	TAILQ_INSERT_TAIL(&(e->env_msg_list), m, msg_link);
	return msg2id(m);

}

int sys_msg_recv(u_int dstva) {
	struct Msg *m;
	struct Page *p;

	if (dstva != 0 && is_illegal_va(dstva)) {
		return -E_INVAL;
	}
	if (TAILQ_EMPTY(&curenv->env_msg_list)) {
		return -E_NO_MSG;
	}

	/* Your Code Here (2/3) */
	m = TAILQ_FIRST(&(curenv->env_msg_list));
	TAILQ_REMOVE(&(curenv->env_msg_list), m, msg_link);
	if (m->msg_page != NULL && dstva != 0) {
		p = m->msg_page;
		try(page_insert(curenv->env_pgdir, curenv->env_asid, p, dstva, m->msg_perm));
		page_decref(p);
	} else if (m->msg_page != NULL) {
		page_decref(m->msg_page);
	}
	
	curenv->env_msg_value = m->msg_value;
	curenv->env_msg_from = m->msg_from;
	curenv->env_msg_perm = m->msg_perm;

	m->msg_status = MSG_RECV;
	TAILQ_INSERT_TAIL(&msg_free_list, m, msg_link);
	return 0;
}
```

### 2.3 2-exam

这部分要实现这样一个功能，在子进程执行系统调用之前，将系统调用号信息发送给父进程，然后父进程接收系统调用号信息并执行相应操作。总体来说，难度不算高，但是需要注意在 `strace_send` 函数中执行系统调用之前，将 `straced` 设置为 `0` 避免循环调用。

```c
void strace_send(int sysno) {
	if (!((SYS_putchar <= sysno && sysno <= SYS_set_tlb_mod_entry) ||
	      (SYS_exofork <= sysno && sysno <= SYS_panic)) ||
	    sysno == SYS_set_trapframe) {
		return;
	}

	// Your code here. (1/2)
	if (straced != 0) {
		straced = 0;
		struct Env *child = envs + ENVX(syscall_getenvid());
		u_int parent = child->env_parent_id;
		ipc_send(parent, sysno, NULL, 0);
		syscall_set_env_status(0, ENV_NOT_RUNNABLE);
		straced = 1;
	}
}

void strace_recv() {
	// Your code here. (2/2)
	int flag = 1;
	u_int child;
	int sysno;
	while (flag) {
		if ((sysno = ipc_recv(&child, NULL, NULL)) == SYS_env_destroy) {
			flag = 0;
		}
		strace_barrier(child);
		recv_sysno(child, sysno);
		syscall_set_env_status(child, ENV_RUNNABLE);
	}
}
```

### 2.4 2-extra

这部分需要实现轻量级进程的创建，功能上和 `fork` 函数类似，但是父进程和子进程完全共享内存，自然也就不再需要写时复制机制。

这道题的难度较高，首先需要在 `kern/syscall_all.c` 中实现内核态系统调用函数 `sys_clone` 并写入系统调用向量表：

```c
int sys_clone(void *func, void *child_stack) {
	struct Page *p = pa2page(PADDR(curenv->env_pgdir));
	if (p->pp_ref >= 64) {
		return -E_ACT_ENV_NUM_EXCEED;
	}
	struct Env *e;
	try(env_clone(&e, curenv->env_id));
	e->env_tf = curenv->env_tf;
	(e->env_tf).regs[29] = child_stack;
	(e->env_tf).cp0_epc = func;
	e->env_status = ENV_RUNNABLE;
	TAILQ_INSERT_TAIL(&env_sched_list, e, env_sched_link);
	return e->env_id;
}
```

然后在 `kern/env.c` 中添加 `env_clone` 函数，这个函数类似 `env_alloc` 函数，只是不再调用 `env_setup_vm` 和 `asid_alloc` 分配相应字段，而是继承父进程的相应字段。

```c
int env_clone(struct Env **new, u_int parent_id) {
	int r;
	struct Env *e;

	if (LIST_EMPTY(&env_free_list)) {
		return -E_NO_FREE_ENV;
	}
	e = LIST_FIRST(&env_free_list);

	e->env_user_tlb_mod_entry = 0;
	e->env_runs = 0;
	e->env_id = mkenvid(e);
	e->env_parent_id = parent_id;

	struct Env *parent = envs + ENVX(parent_id);
	e->env_pgdir = parent->env_pgdir;
	struct Page *p = pa2page(PADDR(e->env_pgdir));
	p->pp_ref++;
	e->env_asid = parent->env_asid;

	e->env_tf.cp0_status = STATUS_IM7 | STATUS_IE | STATUS_EXL | STATUS_UM;
	e->env_tf.regs[29] = USTACKTOP - sizeof(int) - sizeof(char **);

	LIST_REMOVE(e, env_link);

	*new = e;
	return 0;
}
```

还需要修改 `env_free` 的实现，确保只有当该页目录的引用次数为 `1` 时才释放。

其他需要编写的代码就和普通的系统调用函数相同了，包括用户态的系统调用接口等。

还有一点需要说明，页目录本身也会占据一张页面，所以我们使用该页面控制块 `struct Page` 的 `pp_ref` 字段标记页目录的引用次数。

## 3. 实验体会

系统调用是操作系统对用户程序提供服务的重要方式，可以用于保护操作系统的核心功能；同时简化用户程序的实现，编写用户程序时不再需要关心底层实现。

进程通信是多进程环境的重要功能，通过进程通信可以实现进程的同步与互斥，而不是各进程毫无关联，退化为批处理系统。

![彩蛋](pic\彩蛋.png)

另外，这个彩蛋真是太有意思啦！
