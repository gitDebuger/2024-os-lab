#ifndef _ENV_H_
#define _ENV_H_

#include <mmu.h>
#include <queue.h>
#include <trap.h>
#include <types.h>
#include <msg.h>

#define LOG2NENV 10
#define NENV (1 << LOG2NENV) // 进程控制块总数
#define ENVX(envid) ((envid) & (NENV - 1)) // Unknown

// All possible values of 'env_status' in 'struct Env'.
// 进程控制块 Env 结构体中 env_status 字段所有可能的取值
#define ENV_FREE 0 // 空闲
#define ENV_RUNNABLE 1 // 可运行——就绪或正在运行
#define ENV_NOT_RUNNABLE 2 // 阻塞

// Control block of an environment (process).
// 进程控制块结构体
struct Env {
	/* 保存陷入内核时的上下文信息 */
	/* 定义在 /include/trap.h 中 */
	struct Trapframe env_tf;	 // saved context (registers) before switching
	/* env_free_list 链表侵入性节点 */
	/* 用于将该进程的控制块链接到一个名为 env_free_list 的链表中 */
	/* struct { struct Env *le_next; struct Env **le_prev; } */
	LIST_ENTRY(Env) env_link;	 // intrusive entry in 'env_free_list'
	/* 唯一进程标识符 PID */
	u_int env_id;			 // unique environment identifier
	/* 进程地址空间标识符 ASID */
	/* 用于 TLB 中 */
	u_int env_asid;			 // ASID of this env
	/* 进程的父进程 PID */
	u_int env_parent_id;		 // env_id of this env's parent
	/* 进程当前状态 */
	/* 取值见前文宏定义 */
	u_int env_status;		 // status of this env
	/* 当前进程拥有的页目录虚拟地址 */
	Pde *env_pgdir;			 // page directory
	/* env_sched_list 链表侵入性节点 */
	/* 用于将该进程的控制块链接到一个名为 env_sched_list 的链表中 */
	/* 这个链表可能用于调度器的进程调度 */
	/* struct {  struct Env *tqe_next;  struct Env * *tqe_prev; } */
	TAILQ_ENTRY(Env) env_sched_link; // intrusive entry in 'env_sched_list'
	/* 当前进程优先级 */
	u_int env_pri;			 // schedule priority

	// Lab 4 IPC
	/* Lab 4 进程间通信  */

	/* 发送给该进程的值 */
	u_int env_ipc_value;   // the value sent to us
	/* 发送者的 envid 也就是发送者的 PID */
	u_int env_ipc_from;    // envid of the sender
	/* 该进程是否正在阻塞接收消息 */
	u_int env_ipc_recving; // whether this env is blocked receiving
	/* 接收到的页面应该被映射到的虚拟地址 */
	u_int env_ipc_dstva;   // va at which the received page should be mapped
	/* 接收到的页面应该被映射到的权限 */
	u_int env_ipc_perm;    // perm in which the received page should be mapped

	// Lab 4 fault handling
	/* Lab 4 错误处理 */

	/* 用户空间 TLV MOD 处理程序 */
	u_int env_user_tlb_mod_entry; // userspace TLB Mod handler

	// Lab 6 scheduler counts
	/* Lab 6 调度器计数 */

	/* 当前进程已经被 env_run 调用的次数 */
	u_int env_runs; // number of times we've been env_run'ed

	// lab4-1-extra
	struct Msg_list env_msg_list;
	u_int env_msg_value;
	u_int env_msg_from;
	u_int env_msg_perm;
};

/* struct Env_list { struct Env *lh_first; } */
LIST_HEAD(Env_list, Env);
/* 双端队列 */
/* struct Env_sched_list {  struct Env *tqh_first;  struct Env * *tqh_last; } */
TAILQ_HEAD(Env_sched_list, Env);
/* 当前进程控制块指针 */
extern struct Env *curenv;		     // the current env
/* 当前可运行进程链表 */
extern struct Env_sched_list env_sched_list; // runnable env list

/* 进程管理相关函数 */
/* 定义在 /kern/env.c 中 */

void env_init(void);
int env_alloc(struct Env **e, u_int parent_id);
void env_free(struct Env *);
struct Env *env_create(const void *binary, size_t size, int priority);
void env_destroy(struct Env *e);

int envid2env(u_int envid, struct Env **penv, int checkperm);
void env_run(struct Env *e) __attribute__((noreturn));

void env_check(void);
void envid2env_check(void);

/* 进程创建 */

/* 带优先级进程创建 */
#define ENV_CREATE_PRIORITY(x, y)                                                                  \
	({                                                                                         \
		extern u_char binary_##x##_start[];                                                \
		extern u_int binary_##x##_size;                                                    \
		env_create(binary_##x##_start, (u_int)binary_##x##_size, y);                       \
	})
/* 不带优先级进程创建 */
/* 默认优先级为 1 */
#define ENV_CREATE(x)                                                                              \
	({                                                                                         \
		extern u_char binary_##x##_start[];                                                \
		extern u_int binary_##x##_size;                                                    \
		env_create(binary_##x##_start, (u_int)binary_##x##_size, 1);                       \
	})

#endif // !_ENV_H_
