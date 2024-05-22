# 添加系统调用的流程

## 1. 信号量

### 1.1 `include/error.h`

在文件中添加对应的错误码

```c
#define E_SEM_NOT_OPEN 14
```

### 1.2 `include/syscall.h`

在文件的枚举类中添加对应的系统调用号

```c
enum {
    SYS_putchar,
    SYS_print_cons,
    SYS_getenvid,
    SYS_yield,
    SYS_env_destroy,
    SYS_set_tlb_mod_entry,
    SYS_mem_alloc,
    SYS_mem_map,
    SYS_mem_unmap,
    SYS_exofork,
    SYS_set_env_status,
    SYS_set_trapframe,
    SYS_panic,
    SYS_ipc_try_send,
    SYS_ipc_recv,
    SYS_cgetc,
    SYS_write_dev,
    SYS_read_dev,
    SYS_sem_open,
    SYS_sem_wait,
    SYS_sem_post,
    SYS_sem_kill,
    MAX_SYSNO,
};
```

### 1.3 `kern/syscall_all.c`

首先在文件中添加对应的内核态系统调用函数

```c
int sems[15];
int sems_valid[15] = {0};

void sys_sem_open(int sem_id, int n) {
    // Lab 4-1-Exam: Your code here. (6/9)
    if (sem_id >= 0 && sem_id < 15 && !sems_valid[sem_id]) {
        sems_valid[sem_id] = 1;
        sems[sem_id] = n;
    }
}

int sys_sem_wait(int sem_id) {
    // Lab 4-1-Exam: Your code here. (7/9)
    if (sem_id < 0 || sem_id >= 15 || !sems_valid[sem_id]) {
        return -E_SEM_NOT_OPEN;
    }
    if (sems[sem_id] == 0) {
        return 0;
    }
    sems[sem_id]--;
    return sems[sem_id] + 1;
}

int sys_sem_post(int sem_id) {
    // Lab 4-1-Exam: Your code here. (8/9)
    if (sem_id < 0 || sem_id >= 15 || !sems_valid[sem_id]) {
                 return -E_SEM_NOT_OPEN;
        }
    sems[sem_id]++;
    return 0;
}

int sys_sem_kill(int sem_id) {
    // Lab 4-1-Exam: Your code here. (9/9)
    if (sem_id < 0 || sem_id >= 15 || !sems_valid[sem_id]) {
        return -E_SEM_NOT_OPEN;
    }
    sems_valid[sem_id] = 0;
    return 0;
}
```

然后在系统调用向量表中添加相应的数据

```c
void *syscall_table[MAX_SYSNO] = {
    [SYS_putchar] = sys_putchar,
    [SYS_print_cons] = sys_print_cons,
    [SYS_getenvid] = sys_getenvid,
    [SYS_yield] = sys_yield,
    [SYS_env_destroy] = sys_env_destroy,
    [SYS_set_tlb_mod_entry] = sys_set_tlb_mod_entry,
    [SYS_mem_alloc] = sys_mem_alloc,
    [SYS_mem_map] = sys_mem_map,
    [SYS_mem_unmap] = sys_mem_unmap,
    [SYS_exofork] = sys_exofork,
    [SYS_set_env_status] = sys_set_env_status,
    [SYS_set_trapframe] = sys_set_trapframe,
    [SYS_panic] = sys_panic,
    [SYS_ipc_try_send] = sys_ipc_try_send,
    [SYS_ipc_recv] = sys_ipc_recv,
    [SYS_cgetc] = sys_cgetc,
    [SYS_write_dev] = sys_write_dev,
    [SYS_read_dev] = sys_read_dev,
    [SYS_sem_open] = sys_sem_open,
    [SYS_sem_wait] = sys_sem_wait,
    [SYS_sem_post] = sys_sem_post,
    [SYS_sem_kill] = sys_sem_kill,
};
```

### 1.4 `user/include/lib.h`

在文件中声明用户态系统调用函数

```c
void syscall_sem_open(int sem_id, int n);
int syscall_sem_wait(int sem_id);
int syscall_sem_post(int sem_id);
int syscall_sem_kill(int sem_id);
```

由于实验用 MOS 操作系统是微内核架构，所以还需要声明信号量服务函数

```c
void sem_open(int sem_id, int n);
int sem_wait(int sem_id);
int sem_post(int sem_id);
int sem_kill(int sem_id);
```

### 1.5 `user/lib/ipc.c`

在文件中实现上面的信号量服务函数

```c
void sem_open(int sem_id, int n) {
    syscall_sem_open(sem_id, n);
}

int sem_wait(int sem_id) {
    int r;
    // Lab 4-1-Exam: Your code here. (1/9)
    // Implement process blocking
    while ((r = syscall_sem_wait(sem_id)) == 0) {
        syscall_yield();
    }
    if (r == -E_SEM_NOT_OPEN) return r;
    return 0;
}

int sem_post(int sem_id) {
    return syscall_sem_post(sem_id);
}

int sem_kill(int sem_id) {
    return syscall_sem_kill(sem_id);
}
```

### 1.6 `user/lib/syscall_lib.c`

在文件中实现用户态系统调用函数

```c
void syscall_sem_open(int sem_id, int n) {
    // Lab 4-1-Exam: Your code here. (2/9)
    msyscall(SYS_sem_open, sem_id, n);
}

int syscall_sem_wait(int sem_id) {
    // Lab 4-1-Exam: Your code here. (3/9)
    return msyscall(SYS_sem_wait, sem_id);
}

int syscall_sem_post(int sem_id) {
    // Lab 4-1-Exam: Your code here. (4/9)
    return msyscall(SYS_sem_post, sem_id);
}

int syscall_sem_kill(int sem_id) {
    // Lab 4-1-Exam: Your code here. (5/9)
    return msyscall(SYS_sem_kill, sem_id);
}
```

## 2. 消息队列

### 2.1 `include/env.h`

在文件中进程控制块结构体中添加一些相关字段

```c
struct Env {
    struct Trapframe env_tf;
    LIST_ENTRY(Env) env_link;
    u_int env_id;
    u_int env_asid;
    u_int env_parent_id;
    u_int env_status;
    Pde *env_pgdir;
    TAILQ_ENTRY(Env) env_sched_link;
    u_int env_pri;
    u_int env_ipc_value;
    u_int env_ipc_from;
    u_int env_ipc_recving;
    u_int env_ipc_dstva;
    u_int env_ipc_perm;
    u_int env_user_tlb_mod_entry;
    u_int env_runs;
    // lab4-1-extra
    struct Msg_list env_msg_list;
    u_int env_msg_value;
    u_int env_msg_from;
    u_int env_msg_perm;
};
```

添加的字段和上面的进程通信字段是含义相同。

### 2.2 `include/error.h`

在文件中添加错误码

```c
#define E_NO_MSG 14
```

### 2.3 `include/msg.h`

这是消息队列的核心文件，有很多相关的内容

```c
#ifndef _MSG_H_
#define _MSG_H_

#include <mmu.h>
#include <queue.h>

// Lab 4-1 Extra
#define LOG2NMSG 5
#define NMSG (1 << LOG2NMSG)
#define MSGX(msgid) ((msgid) & (NMSG - 1))

/* define the status of Msg block */
enum {
    MSG_FREE, // free Msg
    MSG_SENT, // message has been sent but has not been received
    MSG_RECV, // message has been received
};

// Control block of a message.
struct Msg {
    TAILQ_ENTRY(Msg) msg_link;

    u_int msg_tier;
    u_int msg_status;
    u_int msg_value;
    u_int msg_from;
    u_int msg_perm;
    struct Page *msg_page;
};

TAILQ_HEAD(Msg_list, Msg);

extern struct Msg msgs[];
extern struct Msg_list msg_free_list;

void msg_init(void);
u_int msg2id(struct Msg *m);

#endif // !_MSG_H_
```

### 2.4 `include/syscall.h`

在文件的枚举类中添加对应的系统调用号

```c
enum {
    SYS_putchar,
    SYS_print_cons,
    SYS_getenvid,
    SYS_yield,
    SYS_env_destroy,
    SYS_set_tlb_mod_entry,
    SYS_mem_alloc,
    SYS_mem_map,
    SYS_mem_unmap,
    SYS_exofork,
    SYS_set_env_status,
    SYS_set_trapframe,
    SYS_panic,
    SYS_ipc_try_send,
    SYS_ipc_recv,
    SYS_cgetc,
    SYS_write_dev,
    SYS_read_dev,
    SYS_msg_send,
    SYS_msg_recv,
    SYS_msg_status,
    MAX_SYSNO,
};
```

### 2.5 `user/include/lib.h`

声明用户态系统调用函数和消息队列服务函数

```c
int syscall_msg_send(u_int envid, u_int value, const void *srcva, u_int perm);
int syscall_msg_recv(void *dstva);
int syscall_msg_status(u_int msgid);

int msg_send(u_int whom, u_int val, const void *srcva, u_int perm);
int msg_recv(u_int *whom, u_int *value, void *dstva, u_int *perm);
int msg_status(u_int msgid);
```

### 2.6 `user/lib/ipc.c`

实现消息队列服务函数

```c
int msg_send(u_int whom, u_int val, const void *srcva, u_int perm) {
    return syscall_msg_send(whom, val, srcva, perm);
}

int msg_recv(u_int *whom, u_int *val, void *dstva, u_int *perm) {
    int r = syscall_msg_recv(dstva);
    if (r != 0) {
        return r;
    }
    if (whom) {
        *whom = env->env_msg_from;
    }
    if (perm) {
        *perm = env->env_msg_perm;
    }
    if (val) {
        *val = env->env_msg_value;
    }
    return 0;
}

int msg_status(u_int msgid) {
    return syscall_msg_status(msgid);
}
```

### 2.7 `user/lib/syscall_lib.c`

使用用户态系统调用函数

```c
int syscall_msg_send(u_int envid, u_int value, const void *srcva, u_int perm) {
    return msyscall(SYS_msg_send, envid, value, srcva, perm);
}

int syscall_msg_recv(void *dstva) {
    return msyscall(SYS_msg_recv, dstva);
}

int syscall_msg_status(u_int msgid) {
    return msyscall(SYS_msg_status, msgid);
}
```

### 2.8 `kern/syscall_all.c`

我们将在这里实现内核态系统调用函数，在实现之前，我们先在系统调用向量表中添加对应项：

```c
void *syscall_table[MAX_SYSNO] = {
    [SYS_putchar] = sys_putchar,
    [SYS_print_cons] = sys_print_cons,
    [SYS_getenvid] = sys_getenvid,
    [SYS_yield] = sys_yield,
    [SYS_env_destroy] = sys_env_destroy,
    [SYS_set_tlb_mod_entry] = sys_set_tlb_mod_entry,
    [SYS_mem_alloc] = sys_mem_alloc,
    [SYS_mem_map] = sys_mem_map,
    [SYS_mem_unmap] = sys_mem_unmap,
    [SYS_exofork] = sys_exofork,
    [SYS_set_env_status] = sys_set_env_status,
    [SYS_set_trapframe] = sys_set_trapframe,
    [SYS_panic] = sys_panic,
    [SYS_ipc_try_send] = sys_ipc_try_send,
    [SYS_ipc_recv] = sys_ipc_recv,
    [SYS_cgetc] = sys_cgetc,
    [SYS_write_dev] = sys_write_dev,
    [SYS_read_dev] = sys_read_dev,
    [SYS_msg_send] = sys_msg_send,
    [SYS_msg_recv] = sys_msg_recv,
    [SYS_msg_status] = sys_msg_status,
};
```

然后具体实现这三个系统调用函数：

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

int sys_msg_status(u_int msgid) {
    struct Msg *m;

    /* Your Code Here (3/3) */
    m = &msgs[MSGX(msgid)];
    u_int id = msg2id(m);
    if (id == msgid) {
        return m->msg_status;
    } else if (id > msgid) {
        return MSG_RECV;
    } else {
        return -E_INVAL;
    }
}
```

获取消息状态是很简单的，重点是消息的发送和消息的接收。

这两个函数的实现可以参考同文件中的 `sys_ipc_try_send` 和 `sys_ipc_recv` 两个函数。

在消息发送函数中，我们将需要传送的信息放到消息控制块中，而不是直接设置对方进程控制块的字段值。

在消息接收函数中，我们需要将消息控制块中的信息复制到进程控制块中。

也就是说将原来发送消息的函数的功能拆分成两部分，分配到消息发送和消息接收两个函数中。

而原来的消息接收函数的绝大部分功能都不再需要。
