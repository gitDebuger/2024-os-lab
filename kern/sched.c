#include <env.h>
#include <pmap.h>
#include <printk.h>

/* Overview:
 *   Implement a round-robin scheduling to select a runnable env and schedule it using 'env_run'.
 *
 * Post-Condition:
 *   If 'yield' is set (non-zero), 'curenv' should not be scheduled again unless it is the only
 *   runnable env.
 *
 * Hints:
 *   1. The variable 'count' used for counting slices should be defined as 'static'.
 *   2. Use variable 'env_sched_list', which contains and only contains all runnable envs.
 *   3. You shouldn't use any 'return' statement because this function is 'noreturn'.
 */
/**
 * 实现一个循环调度来选择一个可运行的进程并使用 env_run 来调度它
 * 如果 yield 非零则 curenv 不应该再次被调度除非它是唯一的可运行进程
 * 用来记录时间片的变量 slice 应该被设置为 static
 * 使用包含且仅包含可运行进程控制块的变量 env_sched_list
 * 不应该使用任何 return 语句因为这是一个 noreturn 函数
*/
void schedule(int yield) {
	static int count = 0; // remaining time slices of current env
	struct Env *e = curenv;

	/* We always decrease the 'count' by 1.
	 *
	 * If 'yield' is set, or 'count' has been decreased to 0, or 'e' (previous 'curenv') is
	 * 'NULL', or 'e' is not runnable, then we pick up a new env from 'env_sched_list' (list of
	 * all runnable envs), set 'count' to its priority, and schedule it with 'env_run'. **Panic
	 * if that list is empty**.
	 *
	 * (Note that if 'e' is still a runnable env, we should move it to the tail of
	 * 'env_sched_list' before picking up another env from its head, or we will schedule the
	 * head env repeatedly.)
	 *
	 * Otherwise, we simply schedule 'e' again.
	 *
	 * You may want to use macros below:
	 *   'TAILQ_FIRST', 'TAILQ_REMOVE', 'TAILQ_INSERT_TAIL'
	 */
	/**
	 * 每次调用时 count 自减 1
	 * 如果 yield 非零或者 count 已经被减小到 0 或者 e 也就是前一个 curenv 为空或者 e 不可运行
	 * 则从 env_sched_list 中选取一个新的进程控制块
	 * 将 count 设置为该进程控制块的优先级
	 * 然后使用 env_run 调度该进程
	 * 当 env_sched_list 为空时调用 panic
	 * 注意如果 e 仍然是可运行进程控制块
	 * 我们应该把它移动到 env_sched_list 的末尾在从链表头选取另一个新的进程控制块之前
	 * 否则我们会重复选取链表头的进程控制块
	 * 
	 * 如果不满足切换进程的条件则我们仍然运行当前进程
	 * 可能使用到的宏有
	 * TAILQ_FIRST  TAILQ_REMOVE  TAILQ_INSERT_TAIL
	*/
	/* Exercise 3.12: Your code here. */
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
		e->env_ipc_value++;
		count = e->env_pri;
	}
	count--;
	if (e->env_runs != 0)
		e->env_ipc_from = ((struct Trapframe *)KSTACKTOP - 1)->cp0_clock;
	else
		e->env_ipc_from = 0;
	env_run(e);
}
