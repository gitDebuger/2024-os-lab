#include <env.h>
#include <pmap.h>
#include <printk.h>
#include <trap.h>

/* 声明了一些异常处理函数 */

extern void handle_int(void);
extern void handle_tlb(void);
extern void handle_sys(void);
extern void handle_mod(void);
extern void handle_reserved(void);

/* 异常向量组 */
/* 存放了异常处理函数的地址 */
void (*exception_handlers[32])(void) = {
    /* 默认的保留异常处理函数 */
    [0 ... 31] = handle_reserved,
    /* 中断异常使用 handle_int */
    [0] = handle_int,
    /* TLB 异常使用 handle_tlb */
    [2 ... 3] = handle_tlb,
#if !defined(LAB) || LAB >= 4
    [1] = handle_mod,
    [8] = handle_sys,
#endif
};

/* Overview:
 *   The fallback handler when an unknown exception code is encountered.
 *   'genex.S' wraps this function in 'handle_reserved'.
 */
/* 定义保留异常处理函数 */
/* 当遇到未知的异常号时会调用这个函数 */
void do_reserved(struct Trapframe *tf) {
    /* 首先打印异常帧的信息 */
	print_tf(tf);
    /* 然后通过panic函数输出错误信息并终止程序执行 */
	panic("Unknown ExcCode %2d", (tf->cp0_cause >> 2) & 0x1f);
}
