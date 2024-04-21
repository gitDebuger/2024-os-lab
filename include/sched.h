#ifndef __SCHED_H__
#define __SCHED_H__

/* 定义在 /kern/sched.c 中 */
void schedule(int yield) __attribute__((noreturn));

#endif /* __SCHED_H__ */
